#include "io/MidiImport.h"

#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

#include <vector>

namespace drumprog::io
{
namespace
{

constexpr std::int64_t kBar = 3840;

class MidiImportTest : public ::testing::Test
{
protected:
    MidiFileContent
    fileWith(std::vector<MidiNote> notes, int ticksPerQuarter = 960, std::int64_t length = kBar)
    {
        MidiFileContent content;
        content.ticksPerQuarter = ticksPerQuarter;
        content.lengthTicks = length;
        content.notes = std::move(notes);
        return content;
    }

    model::FakeIdGenerator ids;
    model::Project project{model::ProjectFactory{ids}.createDefault(), nullptr};
    juce::ValueTree kitTree = model::ProjectFactory::createDefaultKit(); // GM notes 35-59
    model::Kit kit{kitTree, nullptr};
};

TEST_F(MidiImportTest, FMI03_KeepsTheNotesOfAFileInTheProjectResolution)
{
    const auto imported =
        importedPatternOf(fileWith({{.note = 36, .startTick = 0, .lengthTicks = 240, .velocity = 100},
                                    {.note = 38, .startTick = 1013, .lengthTicks = 200, .velocity = 64}}),
                          project,
                          kit);

    ASSERT_EQ(imported.notes.size(), 2U);
    EXPECT_EQ(imported.notes[0].slotNote, 36);
    EXPECT_EQ(imported.notes[0].startTick, 0);
    EXPECT_EQ(imported.notes[0].lengthTicks, 240);
    EXPECT_EQ(imported.notes[0].velocity, 100);
    EXPECT_EQ(imported.notes[1].slotNote, 38);
    EXPECT_EQ(imported.notes[1].startTick, 1013);
    EXPECT_EQ(imported.notes[1].lengthTicks, 200);
    EXPECT_EQ(imported.notes[1].velocity, 64);
    EXPECT_EQ(imported.lengthBars, 1);
    EXPECT_TRUE(imported.unmatchedNotes.empty());
    EXPECT_EQ(imported.numUnmatched, 0);
}

TEST_F(MidiImportTest, FMI03_ConvertsTicksToTheProjectResolution)
{
    const auto imported =
        importedPatternOf(fileWith({{.note = 42, .startTick = 1, .lengthTicks = 60, .velocity = 90},
                                    {.note = 42, .startTick = 241, .lengthTicks = 1, .velocity = 90}},
                                   96,
                                   4 * 96),
                          project,
                          kit);

    ASSERT_EQ(imported.notes.size(), 2U);
    EXPECT_EQ(imported.notes[0].startTick, 10);
    EXPECT_EQ(imported.notes[0].lengthTicks, 600);
    EXPECT_EQ(imported.notes[1].startTick, 2410);
    EXPECT_EQ(imported.notes[1].lengthTicks, 10);
}

TEST_F(MidiImportTest, FMI03_RoundsToTheNearestTick)
{
    const auto imported = importedPatternOf(
        fileWith({{.note = 36, .startTick = 1, .lengthTicks = 2, .velocity = 90}}, 1920), project, kit);

    EXPECT_EQ(imported.notes.at(0).startTick, 1);
    EXPECT_EQ(imported.notes.at(0).lengthTicks, 1);
}

TEST_F(MidiImportTest, FMI03_ANoteLastsAtLeastOneTick)
{
    const auto imported = importedPatternOf(
        fileWith({{.note = 36, .startTick = 0, .lengthTicks = 0, .velocity = 90}}), project, kit);

    EXPECT_EQ(imported.notes.at(0).lengthTicks, 1);
}

TEST_F(MidiImportTest, FMI04_ImportedNotesAreLiveNotes)
{
    const auto imported = importedPatternOf(
        fileWith({{.note = 36, .startTick = 0, .lengthTicks = 240, .velocity = 90}}), project, kit);

    EXPECT_EQ(imported.notes.at(0).origin, model::NoteOrigin::live);
}

TEST_F(MidiImportTest, FMI03_ThePatternLastsToTheEndOfTheFileInWholeBars)
{
    EXPECT_EQ(importedPatternOf(fileWith({}, 960, 2 * kBar), project, kit).lengthBars, 2);
    EXPECT_EQ(importedPatternOf(fileWith({}, 960, 2 * kBar + 1), project, kit).lengthBars, 3);
    EXPECT_EQ(importedPatternOf(fileWith({}, 960, 0), project, kit).lengthBars, 1);

    project.setTimeSignature({.numerator = 3, .denominator = 4});
    EXPECT_EQ(importedPatternOf(fileWith({}, 960, kBar), project, kit).lengthBars, 2);
}

TEST_F(MidiImportTest, FMI03_ANoteStartingAtTheEndOfTheFileGetsItsBar)
{
    const auto imported = importedPatternOf(
        fileWith({{.note = 49, .startTick = kBar, .lengthTicks = 0, .velocity = 90}}, 960, kBar),
        project,
        kit);

    EXPECT_EQ(imported.lengthBars, 2);
}

TEST_F(MidiImportTest, FMI03_FindsTheSlotByItsTriggerNoteIfNoSlotHasTheGmNote)
{
    kit.findSlot(38)->setMidiNote(70);

    const auto imported = importedPatternOf(
        fileWith({{.note = 70, .startTick = 0, .lengthTicks = 240, .velocity = 90}}), project, kit);

    ASSERT_EQ(imported.notes.size(), 1U);
    EXPECT_EQ(imported.notes[0].slotNote, 38);
}

TEST_F(MidiImportTest, FMI03_TheGmNoteWinsOverAnotherSlotsTriggerNote)
{
    kit.findSlot(38)->setMidiNote(36);

    const auto imported = importedPatternOf(
        fileWith({{.note = 36, .startTick = 0, .lengthTicks = 240, .velocity = 90}}), project, kit);

    EXPECT_EQ(imported.notes.at(0).slotNote, 36);
}

TEST_F(MidiImportTest, FMI03_ReportsNotesWithoutASlot)
{
    const auto imported =
        importedPatternOf(fileWith({{.note = 61, .startTick = 0, .lengthTicks = 1, .velocity = 90},
                                    {.note = 36, .startTick = 0, .lengthTicks = 1, .velocity = 90},
                                    {.note = 60, .startTick = 10, .lengthTicks = 1, .velocity = 90},
                                    {.note = 61, .startTick = 20, .lengthTicks = 1, .velocity = 90}}),
                          project,
                          kit);

    ASSERT_EQ(imported.notes.size(), 1U);
    EXPECT_EQ(imported.notes[0].slotNote, 36);
    EXPECT_EQ(imported.unmatchedNotes, (std::vector<int>{60, 61}));
    EXPECT_EQ(imported.numUnmatched, 3);
}

} // namespace
} // namespace drumprog::io
