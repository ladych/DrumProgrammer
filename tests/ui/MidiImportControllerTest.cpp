#include "ui/MidiImportController.h"

#include "io/MidiFileWriter.h"
#include "io/MockFileSystem.h"
#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "ui/ActivePattern.h"
#include "ui/FakeMidiImportView.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::ui
{
namespace
{

namespace fs = std::filesystem;
using ::testing::HasSubstr;
using ::testing::Return;
using ::testing::StrictMock;

constexpr std::int64_t kBar = 3840;

class MidiImportControllerTest : public ::testing::Test
{
protected:
    MidiImportControllerTest() { midi.ticksPerQuarter = 480; }

    /// Lets the file dialog choose a file whose content is the MIDI file of `midi`.
    void chooseFile(const fs::path& file)
    {
        view.fileToOpen = file;
        EXPECT_CALL(fileSystem, readText(file)).WillOnce(Return(io::writeMidiFile(midi)));
    }

    [[nodiscard]] int numPatterns() const { return project.numPatterns(); }
    [[nodiscard]] model::Pattern imported() const { return project.pattern(project.numPatterns() - 1); }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();     // "Pattern 1" is id-1
    juce::ValueTree globalKit = model::ProjectFactory::createDefaultKit(); // GM notes 35-59
    model::Project project{tree, nullptr};
    juce::UndoManager undoManager;
    ActivePattern activePattern{tree};
    StrictMock<io::MockFileSystem> fileSystem;
    FakeMidiImportView view;
    MidiImportController controller{tree, globalKit, undoManager, ids, activePattern, fileSystem, view};
    io::MidiPattern midi;
};

TEST_F(MidiImportControllerTest, FMI03_DoesNothingWhenTheDialogIsCancelled)
{
    controller.importMidiFile();

    EXPECT_EQ(view.numFileDialogs, 1);
    EXPECT_EQ(numPatterns(), 1);
    EXPECT_TRUE(view.messages.empty());
}

TEST_F(MidiImportControllerTest, FMI03_AddsTheNotesAsANewPatternNamedAfterTheTrack)
{
    midi.name = "Groove";
    midi.lengthTicks = 6 * 480;
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100},
                  {.note = 38, .startTick = 485, .lengthTicks = 120, .velocity = 70}};
    chooseFile(fs::path{"imports"} / "file.mid");

    controller.importMidiFile();

    ASSERT_EQ(numPatterns(), 2);
    const auto pattern = imported();
    EXPECT_EQ(pattern.id(), "id-2");
    EXPECT_EQ(pattern.name(), "Groove");
    EXPECT_EQ(pattern.lengthBars(), 2);
    ASSERT_EQ(pattern.numNotes(), 2);
    EXPECT_EQ(pattern.note(0).slotNote(), 36);
    EXPECT_EQ(pattern.note(1).slotNote(), 38);
    EXPECT_EQ(pattern.note(1).startTick(), 970);
    EXPECT_EQ(pattern.note(1).lengthTicks(), 240);
    EXPECT_EQ(pattern.note(1).velocity(), 70);
    EXPECT_EQ(pattern.note(1).origin(), model::NoteOrigin::live);
    EXPECT_TRUE(view.messages.empty());
}

TEST_F(MidiImportControllerTest, FMI03_TheImportedPatternBecomesActive)
{
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");

    controller.importMidiFile();

    EXPECT_EQ(activePattern.index(), 1);
}

TEST_F(MidiImportControllerTest, FMI03_NamesThePatternAfterTheFileWithoutTrackName)
{
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile(fs::path{"imports"} / "Beat 7.mid");

    controller.importMidiFile();

    EXPECT_EQ(imported().name(), "Beat 7");
}

TEST_F(MidiImportControllerTest, FMI03_TheImportIsOneUndoStep)
{
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100},
                  {.note = 38, .startTick = 480, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");

    controller.importMidiFile();

    EXPECT_EQ(undoManager.getUndoDescription(), MidiImportController::kUndoName);
    undoManager.undo();
    EXPECT_EQ(numPatterns(), 1);
    EXPECT_EQ(activePattern.index(), 0);
}

TEST_F(MidiImportControllerTest, FMI03_KeepsTempoAndTimeSignatureOfTheProject)
{
    midi.bpm = 90.0;
    midi.timeSigNumerator = 3;
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");

    controller.importMidiFile();

    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);
    EXPECT_EQ(project.timeSignature().numerator, 4);
}

TEST_F(MidiImportControllerTest, FMI03_UsesTheKitTheProjectPlaysWith)
{
    auto ownKit = model::ProjectFactory::createDefaultKit();
    model::Kit{ownKit, nullptr}.findSlot(38)->setMidiNote(70);
    project.setOwnKit(ownKit);
    midi.notes = {{.note = 70, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");

    controller.importMidiFile();

    EXPECT_EQ(imported().note(0).slotNote(), 38);
}

TEST_F(MidiImportControllerTest, FMI03_ReportsNotesWithoutASlot)
{
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100},
                  {.note = 61, .startTick = 0, .lengthTicks = 120, .velocity = 100},
                  {.note = 60, .startTick = 240, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");

    controller.importMidiFile();

    EXPECT_EQ(numPatterns(), 2);
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "MIDI-Import");
    EXPECT_EQ(view.messages[0].text,
              "2 Noten ohne passenden Slot im Kit wurden nicht übernommen (MIDI-Noten 60, 61).");
}

TEST_F(MidiImportControllerTest, FMI03_ReportsASingleNoteWithoutASlot)
{
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100},
                  {.note = 81, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");

    controller.importMidiFile();

    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].text,
              "1 Note ohne passenden Slot im Kit wurde nicht übernommen (MIDI-Note 81).");
}

TEST_F(MidiImportControllerTest, FMI03_AddsNothingIfNoNoteHasASlot)
{
    midi.notes = {{.note = 60, .startTick = 0, .lengthTicks = 120, .velocity = 100},
                  {.note = 60, .startTick = 240, .lengthTicks = 120, .velocity = 100}};
    chooseFile("piano.mid");

    controller.importMidiFile();

    EXPECT_EQ(numPatterns(), 1);
    EXPECT_FALSE(undoManager.canUndo());
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Import fehlgeschlagen");
    EXPECT_EQ(view.messages[0].text, "Keine Note aus „piano.mid“ passt zu einem Slot im Kit (MIDI-Note 60).");
}

TEST_F(MidiImportControllerTest, FMI03_AddsNothingForAFileWithoutNotes)
{
    midi.lengthTicks = 4 * 480;
    chooseFile(fs::path{"imports"} / "tempo.mid");

    controller.importMidiFile();

    EXPECT_EQ(numPatterns(), 1);
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Import fehlgeschlagen");
    EXPECT_EQ(view.messages[0].text, "„tempo.mid“ enthält keine Noten.");
}

TEST_F(MidiImportControllerTest, FMI03_ReportsAFileThatCannotBeRead)
{
    view.fileToOpen = fs::path{"gone.mid"};
    EXPECT_CALL(fileSystem, readText(fs::path{"gone.mid"})).WillOnce(Return(std::nullopt));

    controller.importMidiFile();

    EXPECT_EQ(numPatterns(), 1);
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Import fehlgeschlagen");
    EXPECT_THAT(view.messages[0].text, HasSubstr("„gone.mid“ ist keine lesbare MIDI-Datei"));
}

TEST_F(MidiImportControllerTest, FMI03_ReportsAFileThatIsNoMidiFile)
{
    view.fileToOpen = fs::path{"notes.txt"};
    EXPECT_CALL(fileSystem, readText(fs::path{"notes.txt"})).WillOnce(Return(std::string{"Hallo"}));

    controller.importMidiFile();

    EXPECT_EQ(numPatterns(), 1);
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_THAT(view.messages[0].text, HasSubstr("„notes.txt“ ist keine lesbare MIDI-Datei"));
}

} // namespace
} // namespace drumprog::ui
