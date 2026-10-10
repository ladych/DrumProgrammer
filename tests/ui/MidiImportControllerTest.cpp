#include "ui/MidiImportController.h"

#include "io/MidiFileWriter.h"
#include "io/MockFileSystem.h"
#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "ui/ActivePattern.h"
#include "ui/FakeMidiImportView.h"
#include "ui/TempoPresenter.h"

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
    MidiImportControllerTest()
    {
        midi.ticksPerQuarter = 480;
        midi.lengthTicks = 4 * 480;
    }

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

TEST_F(MidiImportControllerTest, FMI03_AsksWhetherToTakeTempoAndTimeSignatureOfTheFile)
{
    midi.bpm = 87.5;
    midi.timeSigNumerator = 3;
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");

    controller.importMidiFile();

    ASSERT_EQ(view.questions.size(), 1U);
    EXPECT_EQ(view.questions[0],
              "Tempo der Datei: 87.50 BPM (Projekt: 120.00 BPM)\nTaktart der Datei: 3/4 (Projekt: 4/4)\n\n"
              "Ins Projekt übernehmen?");
}

TEST_F(MidiImportControllerTest, FMI03_AsksOnlyAboutWhatDiffers)
{
    midi.bpm = 90.0;
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");
    controller.importMidiFile();

    midi.bpm = 120.0;
    midi.timeSigNumerator = 7;
    midi.timeSigDenominator = 8;
    chooseFile("odd.mid");
    controller.importMidiFile();

    midi.timeSigNumerator = 4;
    chooseFile("eighths.mid");
    controller.importMidiFile();

    ASSERT_EQ(view.questions.size(), 3U);
    EXPECT_EQ(view.questions[2], "Taktart der Datei: 4/8 (Projekt: 4/4)\n\nIns Projekt übernehmen?");
    EXPECT_EQ(view.questions[0],
              "Tempo der Datei: 90.00 BPM (Projekt: 120.00 BPM)\n\nIns Projekt übernehmen?");
    EXPECT_EQ(view.questions[1], "Taktart der Datei: 7/8 (Projekt: 4/4)\n\nIns Projekt übernehmen?");
}

TEST_F(MidiImportControllerTest, FMI03_DoesNotAskIfTempoAndTimeSignatureMatch)
{
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");

    controller.importMidiFile();

    EXPECT_TRUE(view.questions.empty());
    EXPECT_EQ(numPatterns(), 2);
}

TEST_F(MidiImportControllerTest, FMI03_DoesNotAskAboutAnInvalidTimeSignature)
{
    midi.timeSigNumerator = 5;
    midi.timeSigDenominator = 2;
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");
    controller.importMidiFile();

    midi.timeSigNumerator = 17;
    midi.timeSigDenominator = 4;
    chooseFile("long.mid");
    controller.importMidiFile();

    EXPECT_TRUE(view.questions.empty());
}

TEST_F(MidiImportControllerTest, FMI03_TakesTempoAndTimeSignatureIfTheUserAgrees)
{
    view.adoptTempo = true;
    midi.bpm = 87.5;
    midi.timeSigNumerator = 3;
    midi.lengthTicks = 12 * 480;
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("waltz.mid");

    controller.importMidiFile();

    EXPECT_DOUBLE_EQ(project.bpm(), 87.5);
    EXPECT_EQ(project.timeSignature().numerator, 3);
    EXPECT_EQ(project.timeSignature().denominator, 4);
    EXPECT_EQ(imported().lengthBars(), 4); // 12 quarters in 3/4
}

TEST_F(MidiImportControllerTest, FMI03_KeepsTempoAndTimeSignatureIfTheUserDeclines)
{
    midi.bpm = 87.5;
    midi.timeSigNumerator = 3;
    midi.lengthTicks = 12 * 480;
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("waltz.mid");

    controller.importMidiFile();

    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);
    EXPECT_EQ(project.timeSignature().numerator, 4);
    EXPECT_EQ(numPatterns(), 2);
    EXPECT_EQ(imported().lengthBars(), 3); // 12 quarters in 4/4
}

TEST_F(MidiImportControllerTest, FMI03_RoundsAndClampsTheTempoOfTheFile)
{
    view.adoptTempo = true;
    midi.bpm = 96.5; // stored as 621762 microseconds per quarter
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");
    controller.importMidiFile();
    EXPECT_DOUBLE_EQ(project.bpm(), 96.5);

    midi.bpm = 20.0;
    chooseFile("slow.mid");
    controller.importMidiFile();
    EXPECT_DOUBLE_EQ(project.bpm(), TempoPresenter::kMinBpm);
}

TEST_F(MidiImportControllerTest, FMI03_KeepsTheProjectTempoIfTheFileHasNone)
{
    // Type 0, 480 ticks per quarter, one note on channel 10 and the end of track, no tempo
    const std::string file = std::string{"MThd\0\0\0\x06\0\0\0\x01\x01\xE0MTrk\0\0\0\x0C", 22} +
                             std::string{"\0\x99\x24\x64\x60\x89\x24\0\0\xFF\x2F\0", 12};
    view.fileToOpen = fs::path{"beat.mid"};
    EXPECT_CALL(fileSystem, readText(fs::path{"beat.mid"})).WillOnce(Return(file));

    controller.importMidiFile();

    EXPECT_TRUE(view.questions.empty());
    EXPECT_EQ(numPatterns(), 2);
    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);
}

TEST_F(MidiImportControllerTest, FMI03_TheTakenTempoIsPartOfTheImportUndoStep)
{
    view.adoptTempo = true;
    midi.bpm = 90.0;
    midi.timeSigNumerator = 3;
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("beat.mid");

    controller.importMidiFile();
    undoManager.undo();

    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);
    EXPECT_EQ(project.timeSignature().numerator, 4);
    EXPECT_EQ(numPatterns(), 1);
}

TEST_F(MidiImportControllerTest, FMI03_AnImportIntoAnEmptySongBecomesItsFirstBlock)
{
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("song.mid");

    controller.importMidiFile();

    ASSERT_EQ(project.song().numEntries(), 1);
    EXPECT_EQ(project.song().entry(0).patternId(), imported().id());
    EXPECT_EQ(project.song().entry(0).startBar(), 0);
    undoManager.undo();
    EXPECT_EQ(project.song().numEntries(), 0);
}

TEST_F(MidiImportControllerTest, FMI03_AnImportLeavesASongWithBlocksAsItIs)
{
    project.song().addEntry("id-1", 0);
    midi.notes = {{.note = 36, .startTick = 0, .lengthTicks = 120, .velocity = 100}};
    chooseFile("fill.mid");

    controller.importMidiFile();

    ASSERT_EQ(project.song().numEntries(), 1);
    EXPECT_EQ(project.song().entry(0).patternId(), "id-1");
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
