#include "io/PatternMidiExport.h"

#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace drumprog::io
{
namespace
{

namespace fs = std::filesystem;

class PatternMidiExportTest : public ::testing::Test
{
protected:
    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    model::Project project{tree, nullptr};
};

TEST_F(PatternMidiExportTest, FMI01_TakesTempoTimeSignatureAndResolutionOfTheProject)
{
    project.setBpm(96.5);
    project.setTimeSignature({.numerator = 6, .denominator = 8});
    const auto pattern = project.addPattern("p-2", "Shuffle", 3);

    const auto midi = midiPatternOf(project, pattern);

    EXPECT_EQ(midi.name, "Shuffle");
    EXPECT_DOUBLE_EQ(midi.bpm, 96.5);
    EXPECT_EQ(midi.timeSigNumerator, 6);
    EXPECT_EQ(midi.timeSigDenominator, 8);
    EXPECT_EQ(midi.ticksPerQuarter, 960);
    EXPECT_EQ(midi.lengthTicks, 3 * 6 * 480);
}

TEST_F(PatternMidiExportTest, FMI01_ExportsEveryNoteWithTheGmNoteOfItsSlot)
{
    auto pattern = project.pattern(0);
    pattern.addNote({.slotNote = 38, .startTick = 960, .lengthTicks = 240, .velocity = 90});
    pattern.addNote({.slotNote = 36,
                     .startTick = 13,
                     .lengthTicks = 200,
                     .velocity = 127,
                     .origin = model::NoteOrigin::live});

    const auto midi = midiPatternOf(project, pattern);

    ASSERT_EQ(midi.notes.size(), 2U);
    EXPECT_EQ(midi.notes[0].note, 38);
    EXPECT_EQ(midi.notes[0].startTick, 960);
    EXPECT_EQ(midi.notes[0].lengthTicks, 240);
    EXPECT_EQ(midi.notes[0].velocity, 90);
    EXPECT_EQ(midi.notes[1].note, 36);
    EXPECT_EQ(midi.notes[1].startTick, 13);
    EXPECT_EQ(midi.notes[1].lengthTicks, 200);
    EXPECT_EQ(midi.notes[1].velocity, 127);
}

TEST(MidiFileNameTest, FMI01_AddsTheExtensionMidUnlessTheFileHasIt)
{
    EXPECT_EQ(withMidiExtension("export/Groove"), fs::path{"export/Groove.mid"});
    EXPECT_EQ(withMidiExtension("export/Groove.v2"), fs::path{"export/Groove.v2.mid"});
    EXPECT_EQ(withMidiExtension("export/Groove.mid"), fs::path{"export/Groove.mid"});
    EXPECT_EQ(withMidiExtension("export/Groove.MID"), fs::path{"export/Groove.MID"});
    EXPECT_EQ(withMidiExtension("export/Groove.midi"), fs::path{"export/Groove.midi"});
}

} // namespace
} // namespace drumprog::io
