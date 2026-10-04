#include "model/SnapshotBuilder.h"
#include "TestComparisons.h"
#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace drumprog::model
{
namespace
{

using engine::NoteSnapshot;
using engine::SlotSnapshot;
using engine::SongEntrySnapshot;

class SnapshotBuilderTest : public ::testing::Test
{
protected:
    FakeIdGenerator ids;
    Project project{ProjectFactory{ids}.createDefault(), nullptr};

    static constexpr int kKickIndex = 1;  // GM 36
    static constexpr int kSnareIndex = 3; // GM 38
};

TEST_F(SnapshotBuilderTest, FPJ02_CopiesTempoTimeSignatureAndMix)
{
    project.setBpm(98.25);
    project.setTimeSignature({6, 8});
    project.mix().setBackingGain(0.25);
    project.mix().setDrumsGain(0.5);
    project.mix().setMasterGain(0.75);

    const auto snapshot = SnapshotBuilder::build(project);

    EXPECT_DOUBLE_EQ(snapshot->bpm, 98.25);
    EXPECT_EQ(snapshot->timeSigNumerator, 6);
    EXPECT_EQ(snapshot->timeSigDenominator, 8);
    EXPECT_EQ(snapshot->ticksPerQuarter, 960);
    EXPECT_EQ(snapshot->mix, (engine::MixSnapshot{0.25, 0.5, 0.75}));
}

TEST_F(SnapshotBuilderTest, FSE05_CopiesSlotsInKitOrder)
{
    auto snare = *project.kit().findSlot(38);
    snare.setMidiNote(40);
    snare.setGain(0.5);
    snare.setPitch(2.0);

    const auto snapshot = SnapshotBuilder::build(project);

    ASSERT_EQ(snapshot->slots.size(), 25U);
    EXPECT_EQ(snapshot->slots[kSnareIndex], (SlotSnapshot{40, 0.5, 2.0, 0}));
    EXPECT_EQ(snapshot->slots[7], (SlotSnapshot{42, 1.0, 0.0, 1}));
}

TEST_F(SnapshotBuilderTest, FPJ02_NotesAreSortedByStartAndReferenceSlotsByIndex)
{
    auto pattern = project.pattern(0);
    pattern.addNote({38, 960, 240, 90, NoteOrigin::grid});
    pattern.addNote({36, 0, 240, 100, NoteOrigin::live});
    pattern.addNote({36, 1920, 120, 110, NoteOrigin::grid});

    const auto snapshot = SnapshotBuilder::build(project);

    ASSERT_EQ(snapshot->patterns.size(), 1U);
    const auto& built = snapshot->patterns[0];
    EXPECT_EQ(built.lengthTicks, 2 * 3840);
    ASSERT_EQ(built.notes.size(), 3U);
    EXPECT_EQ(built.notes[0], (NoteSnapshot{kKickIndex, 0, 240, 100}));
    EXPECT_EQ(built.notes[1], (NoteSnapshot{kSnareIndex, 960, 240, 90}));
    EXPECT_EQ(built.notes[2], (NoteSnapshot{kKickIndex, 1920, 120, 110}));
}

TEST_F(SnapshotBuilderTest, FPJ02_SkipsNotesOfUnknownSlots)
{
    project.pattern(0).addNote({80, 0, 240, 100, NoteOrigin::grid});

    const auto snapshot = SnapshotBuilder::build(project);

    EXPECT_TRUE(snapshot->patterns[0].notes.empty());
}

TEST_F(SnapshotBuilderTest, FSO04_SongBlocksAreSortedAndResolvedToPatternIndex)
{
    project.addPattern("verse", "Verse", 4);
    auto song = project.song();
    song.addEntry("verse", 2);
    song.addEntry("id-1", 0);
    song.addEntry("verse", 6);

    const auto snapshot = SnapshotBuilder::build(project);

    ASSERT_EQ(snapshot->song.size(), 3U);
    EXPECT_EQ(snapshot->song[0], (SongEntrySnapshot{0, 0}));
    EXPECT_EQ(snapshot->song[1], (SongEntrySnapshot{1, 2 * 3840}));
    EXPECT_EQ(snapshot->song[2], (SongEntrySnapshot{1, 6 * 3840}));
}

TEST_F(SnapshotBuilderTest, FSO04_SkipsSongBlocksOfUnknownPatterns)
{
    project.song().addEntry("deleted", 0);

    const auto snapshot = SnapshotBuilder::build(project);

    EXPECT_TRUE(snapshot->song.empty());
}

} // namespace
} // namespace drumprog::model
