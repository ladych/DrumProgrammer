#include "io/SongMidiExport.h"

#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace drumprog::io
{
namespace
{

constexpr std::int64_t kBar = 3840;

/// "note@start+length" for every note.
std::vector<std::string> notesOf(const MidiPattern& midi)
{
    std::vector<std::string> result;
    for (const auto& note : midi.notes)
        result.push_back(std::to_string(note.note) + "@" + std::to_string(note.startTick) + "+" +
                         std::to_string(note.lengthTicks));
    return result;
}

class SongMidiExportTest : public ::testing::Test
{
protected:
    SongMidiExportTest()
    {
        // "Pattern 1" (id-1), 2 bars
        project.pattern(0).addNote({.slotNote = 36, .startTick = 0, .lengthTicks = 240, .velocity = 100});
        project.pattern(0).addNote(
            {.slotNote = 38, .startTick = kBar + 960, .lengthTicks = 240, .velocity = 90});
        project.addPattern("fill", "Fill", 1)
            .addNote({.slotNote = 49, .startTick = 480, .lengthTicks = 240, .velocity = 120});
    }

    model::FakeIdGenerator ids;
    model::Project project{model::ProjectFactory{ids}.createDefault(), nullptr};
};

TEST_F(SongMidiExportTest, FMI02_TakesNameTempoTimeSignatureAndResolutionOfTheProject)
{
    project.setName("Mein Song");
    project.setBpm(96.5);
    project.setTimeSignature({.numerator = 7, .denominator = 8});

    const auto midi = midiSongOf(project);

    EXPECT_EQ(midi.name, "Mein Song");
    EXPECT_DOUBLE_EQ(midi.bpm, 96.5);
    EXPECT_EQ(midi.timeSigNumerator, 7);
    EXPECT_EQ(midi.timeSigDenominator, 8);
    EXPECT_EQ(midi.ticksPerQuarter, 960);
}

TEST_F(SongMidiExportTest, FMI02_PlacesTheNotesOfEveryBlockAtItsSongPosition)
{
    project.song().addEntry("fill", 2);
    project.song().addEntry("id-1", 0);
    project.song().addEntry("fill", 4);

    const auto midi = midiSongOf(project);

    EXPECT_EQ(notesOf(midi),
              (std::vector<std::string>{"36@0+240",
                                        "38@" + std::to_string(kBar + 960) + "+240",
                                        "49@" + std::to_string(2 * kBar + 480) + "+240",
                                        "49@" + std::to_string(4 * kBar + 480) + "+240"}));
    EXPECT_EQ(midi.notes[0].velocity, 100);
    EXPECT_EQ(midi.notes[2].velocity, 120);
    EXPECT_EQ(midi.lengthTicks, 5 * kBar);
}

TEST_F(SongMidiExportTest, FMI02_ABlockEndsWhereTheNextOneStarts)
{
    project.pattern(0).addNote({.slotNote = 42, .startTick = kBar - 120, .lengthTicks = 480, .velocity = 80});
    project.song().addEntry("id-1", 0);
    project.song().addEntry("fill", 1);

    const auto midi = midiSongOf(project);

    EXPECT_EQ(notesOf(midi),
              (std::vector<std::string>{"36@0+240",
                                        "42@" + std::to_string(kBar - 120) + "+120",
                                        "49@" + std::to_string(kBar + 480) + "+240"}));
    EXPECT_EQ(midi.lengthTicks, 2 * kBar);
}

TEST_F(SongMidiExportTest, FMI02_NotesBehindThePatternEndAreNotExported)
{
    project.pattern(0).setLengthBars(1);
    project.song().addEntry("id-1", 0);

    EXPECT_EQ(notesOf(midiSongOf(project)), std::vector<std::string>{"36@0+240"});
}

TEST_F(SongMidiExportTest, FMI02_AnEmptySongHasNoNotes)
{
    const auto midi = midiSongOf(project);

    EXPECT_TRUE(midi.notes.empty());
    EXPECT_EQ(midi.lengthTicks, 0);
}

} // namespace
} // namespace drumprog::io
