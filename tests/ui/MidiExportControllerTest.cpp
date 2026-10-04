#include "ui/MidiExportController.h"

#include "io/MockFileSystem.h"
#include "io/SmfReader.h"
#include "model/FakeIdGenerator.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "ui/ActivePattern.h"
#include "ui/FakeMidiExportView.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace drumprog::ui
{
namespace
{

namespace fs = std::filesystem;
using ::testing::_;
using ::testing::DoAll;
using ::testing::HasSubstr;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;

class MidiExportControllerTest : public ::testing::Test
{
protected:
    MidiExportControllerTest()
    {
        project.pattern(0).addNote({.slotNote = 36, .startTick = 0, .lengthTicks = 240, .velocity = 100});
        project.addPattern("p-2", "Fill", 1)
            .addNote({.slotNote = 38, .startTick = 480, .lengthTicks = 240, .velocity = 80});
    }

    /// Note-on events of the written file as "note@tick".
    std::vector<std::string> noteOns() const
    {
        std::vector<std::string> result;
        for (const auto& event : io::readSmf(written).events)
            if (event.bytes.front() == 0x99)
                result.push_back(std::to_string(event.bytes[1]) + "@" + std::to_string(event.tick));
        return result;
    }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    model::Project project{tree, nullptr};
    ActivePattern activePattern{tree};
    StrictMock<io::MockFileSystem> fileSystem;
    FakeMidiExportView view;
    MidiExportController controller{tree, activePattern, fileSystem, view};
    std::string written;
};

TEST_F(MidiExportControllerTest, FMI01_SuggestsTheNameOfTheActivePattern)
{
    activePattern.select(1);

    controller.exportActivePattern();

    EXPECT_EQ(view.suggestedNames, std::vector<std::string>{"Fill.mid"});
}

TEST_F(MidiExportControllerTest, FMI01_WritesTheActivePatternToTheChosenFile)
{
    activePattern.select(1);
    view.fileToSave = fs::path{"export"} / "Fill.mid";
    EXPECT_CALL(fileSystem, writeText(fs::path{"export"} / "Fill.mid", _))
        .WillOnce(DoAll(SaveArg<1>(&written), Return(true)));

    controller.exportActivePattern();

    EXPECT_EQ(noteOns(), std::vector<std::string>{"38@480"});
    EXPECT_TRUE(view.messages.empty());
}

TEST_F(MidiExportControllerTest, FMI01_AddsTheExtensionMid)
{
    view.fileToSave = fs::path{"export"} / "Groove";
    EXPECT_CALL(fileSystem, writeText(fs::path{"export"} / "Groove.mid", _))
        .WillOnce(DoAll(SaveArg<1>(&written), Return(true)));

    controller.exportActivePattern();

    EXPECT_EQ(noteOns(), std::vector<std::string>{"36@0"});
}

TEST_F(MidiExportControllerTest, FMI01_WritesNothingWhenTheDialogIsCancelled)
{
    controller.exportActivePattern();

    EXPECT_EQ(view.suggestedNames.size(), 1U);
    EXPECT_TRUE(view.messages.empty());
}

TEST_F(MidiExportControllerTest, FMI01_ReportsAFileThatCannotBeWritten)
{
    view.fileToSave = fs::path{"readonly"} / "Pattern 1.mid";
    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(Return(false));

    controller.exportActivePattern();

    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Export fehlgeschlagen");
    EXPECT_THAT(view.messages[0].text, HasSubstr("Pattern 1.mid"));
}

TEST_F(MidiExportControllerTest, FMI01_ExportsThePatternThatWasActiveWhenTheDialogOpened)
{
    view.answerLater = true;
    controller.exportActivePattern();
    activePattern.select(1);
    project.setBpm(140.0);
    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(DoAll(SaveArg<1>(&written), Return(true)));

    view.pendingAnswer(fs::path{"Pattern 1.mid"});

    EXPECT_EQ(noteOns(), std::vector<std::string>{"36@0"});
}

TEST_F(MidiExportControllerTest, FMI01_CanExportOnlyWhileAPatternIsActive)
{
    EXPECT_TRUE(controller.canExportPattern());

    tree.getChildWithName(model::ids::patterns).removeAllChildren(nullptr);
    controller.exportActivePattern();

    EXPECT_FALSE(controller.canExportPattern());
    EXPECT_TRUE(view.suggestedNames.empty());
}

} // namespace
} // namespace drumprog::ui
