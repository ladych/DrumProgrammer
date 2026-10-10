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

TEST_F(MidiExportControllerTest, FMI01_TheFileDialogNamesThePattern)
{
    controller.exportActivePattern();

    EXPECT_EQ(view.dialogTitles, std::vector<std::string>{"Pattern als MIDI exportieren"});
}

TEST_F(MidiExportControllerTest, FMI05_AsksWhetherToExportThePatternOrTheSong)
{
    controller.exportMidi();
    project.song().addEntry("p-2", 0);
    controller.exportMidi();

    ASSERT_EQ(view.kindChoices.size(), 2U);
    EXPECT_TRUE(view.kindChoices[0].canExportPattern);
    EXPECT_FALSE(view.kindChoices[0].canExportSong);
    EXPECT_TRUE(view.kindChoices[1].canExportPattern);
    EXPECT_TRUE(view.kindChoices[1].canExportSong);
}

TEST_F(MidiExportControllerTest, FMI05_CancellingTheChoiceWritesNothing)
{
    controller.exportMidi();

    EXPECT_EQ(view.kindChoices.size(), 1U);
    EXPECT_TRUE(view.suggestedNames.empty());
}

TEST_F(MidiExportControllerTest, FMI05_ThePatternChoiceExportsTheActivePattern)
{
    activePattern.select(1);
    view.kindToExport = MidiExportKind::pattern;
    view.fileToSave = fs::path{"Fill.mid"};
    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(DoAll(SaveArg<1>(&written), Return(true)));

    controller.exportMidi();

    EXPECT_EQ(view.suggestedNames, std::vector<std::string>{"Fill.mid"});
    EXPECT_EQ(noteOns(), std::vector<std::string>{"38@480"});
}

TEST_F(MidiExportControllerTest, FMI02_TheSongChoiceExportsTheSongUnderTheProjectName)
{
    project.setName("Mein Song");
    project.song().addEntry("p-2", 1);
    view.kindToExport = MidiExportKind::song;
    view.fileToSave = fs::path{"export"} / "Mein Song";
    EXPECT_CALL(fileSystem, writeText(fs::path{"export"} / "Mein Song.mid", _))
        .WillOnce(DoAll(SaveArg<1>(&written), Return(true)));

    controller.exportMidi();

    EXPECT_EQ(view.dialogTitles, std::vector<std::string>{"Song als MIDI exportieren"});
    EXPECT_EQ(view.suggestedNames, std::vector<std::string>{"Mein Song.mid"});
    EXPECT_EQ(noteOns(), std::vector<std::string>{"38@" + std::to_string(3840 + 480)});
    EXPECT_TRUE(view.messages.empty());
}

TEST_F(MidiExportControllerTest, FMI02_ExportsTheSongAsItIsWhenTheFileIsWritten)
{
    project.song().addEntry("p-2", 0);
    view.answerLater = true;
    controller.exportSong();
    project.song().addEntry(project.pattern(0).id(), 1);
    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(DoAll(SaveArg<1>(&written), Return(true)));

    view.pendingAnswer(fs::path{"Song.mid"});

    EXPECT_EQ(noteOns(), (std::vector<std::string>{"38@480", "36@3840"}));
}

TEST_F(MidiExportControllerTest, FMI02_ReportsASongThatCannotBeWritten)
{
    project.song().addEntry("p-2", 0);
    view.fileToSave = fs::path{"readonly"} / "Song.mid";
    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(Return(false));

    controller.exportSong();

    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Export fehlgeschlagen");
    EXPECT_THAT(view.messages[0].text, HasSubstr("Der Song"));
    EXPECT_THAT(view.messages[0].text, HasSubstr("Song.mid"));
}

TEST_F(MidiExportControllerTest, FMI02_CanExportTheSongOnlyWithABlockOfAKnownPattern)
{
    EXPECT_FALSE(controller.canExportSong());
    project.song().addEntry("deleted", 0);
    EXPECT_FALSE(controller.canExportSong());
    controller.exportSong();
    EXPECT_TRUE(view.suggestedNames.empty());

    project.song().addEntry("p-2", 0);
    EXPECT_TRUE(controller.canExportSong());
}

TEST_F(MidiExportControllerTest, FMI05_OffersNothingWithoutPatternAndSong)
{
    tree.getChildWithName(model::ids::patterns).removeAllChildren(nullptr);

    controller.exportMidi();

    EXPECT_FALSE(controller.canExport());
    EXPECT_TRUE(view.kindChoices.empty());
}

} // namespace
} // namespace drumprog::ui
