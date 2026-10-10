#include "io/MidiFileReader.h"
#include "io/MidiFileWriter.h"
#include "io/MidiImport.h"
#include "io/PatternMidiExport.h"
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

/// "slot@start+length/velocity" for every note.
std::vector<std::string> notesOf(const std::vector<model::NoteData>& notes)
{
    std::vector<std::string> result;
    for (const auto& note : notes)
        result.push_back(std::to_string(note.slotNote) + "@" + std::to_string(note.startTick) + "+" +
                         std::to_string(note.lengthTicks) + "/" + std::to_string(note.velocity));
    return result;
}

class MidiRoundTripTest : public ::testing::Test
{
protected:
    ImportedPattern reimport(const MidiPattern& midi) const
    {
        const auto content = readMidiFile(writeMidiFile(midi));
        EXPECT_TRUE(content.has_value());
        if (!content)
            return {};
        return importedPatternOf(*content, project, kit);
    }

    model::FakeIdGenerator ids;
    model::Project project{model::ProjectFactory{ids}.createDefault(), nullptr};
    juce::ValueTree kitTree = model::ProjectFactory::createDefaultKit();
    model::Kit kit{kitTree, nullptr};
};

TEST_F(MidiRoundTripTest, FMI03_AnExportedPatternImportsWithTheSameNotes)
{
    auto pattern = project.pattern(0); // 2 bars
    pattern.addNote({.slotNote = 36, .startTick = 0, .lengthTicks = 240, .velocity = 100});
    pattern.addNote({.slotNote = 42, .startTick = 13, .lengthTicks = 77, .velocity = 1});
    pattern.addNote({.slotNote = 38, .startTick = kBar + 960, .lengthTicks = 240, .velocity = 127});
    pattern.addNote({.slotNote = 49, .startTick = 2 * kBar - 10, .lengthTicks = 480, .velocity = 90});

    const auto imported = reimport(midiPatternOf(project, pattern));

    EXPECT_EQ(notesOf(imported.notes),
              (std::vector<std::string>{"36@0+240/100",
                                        "42@13+77/1",
                                        "38@" + std::to_string(kBar + 960) + "+240/127",
                                        "49@" + std::to_string(2 * kBar - 10) + "+10/90"}));
    EXPECT_EQ(imported.lengthBars, 2);
    EXPECT_EQ(imported.numUnmatched, 0);
}

TEST_F(MidiRoundTripTest, FMI02_AnExportedSongImportsAsOnePatternOverTheWholeSong)
{
    project.pattern(0).addNote({.slotNote = 36, .startTick = 0, .lengthTicks = 240, .velocity = 100});
    project.addPattern("fill", "Fill", 1)
        .addNote({.slotNote = 49, .startTick = 480, .lengthTicks = 240, .velocity = 120});
    project.song().addEntry("id-1", 0);
    project.song().addEntry("fill", 2);
    project.song().addEntry("id-1", 3);

    const auto imported = reimport(midiSongOf(project));

    EXPECT_EQ(notesOf(imported.notes),
              (std::vector<std::string>{"36@0+240/100",
                                        "49@" + std::to_string(2 * kBar + 480) + "+240/120",
                                        "36@" + std::to_string(3 * kBar) + "+240/100"}));
    EXPECT_EQ(imported.lengthBars, 5);
}

} // namespace
} // namespace drumprog::io
