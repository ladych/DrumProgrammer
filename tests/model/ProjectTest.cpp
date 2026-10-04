#include "model/Project.h"
#include "TestComparisons.h"
#include "model/FakeIdGenerator.h"
#include "model/ModelIds.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace drumprog::model
{
namespace
{

class ProjectTest : public ::testing::Test
{
protected:
    FakeIdGenerator ids;
    juce::UndoManager undoManager;
    juce::ValueTree tree = ProjectFactory{ids}.createDefault();
    Project project{tree, &undoManager};

    Pattern firstPattern() const { return project.pattern(0); }
    SampleSlot snare() const { return *project.kit().findSlot(38); }
};

TEST_F(ProjectTest, FPJ02_ChangesNameTempoAndTimeSignature)
{
    project.setName("Rock-Demo");
    project.setBpm(97.5);
    project.setTimeSignature({7, 8});

    EXPECT_EQ(project.name(), "Rock-Demo");
    EXPECT_DOUBLE_EQ(project.bpm(), 97.5);
    EXPECT_EQ(project.timeSignature(), (TimeSignature{7, 8}));
}

TEST_F(ProjectTest, FPJ02_TicksPerBarFollowTimeSignature)
{
    EXPECT_EQ(project.ticksPerBar(), 3840);

    project.setTimeSignature({7, 8});
    EXPECT_EQ(project.ticksPerBar(), 3360);

    project.setTimeSignature({3, 16});
    EXPECT_EQ(project.ticksPerBar(), 720);
}

TEST_F(ProjectTest, FPJ02_WrapperIsAHandleOntoTheTree)
{
    project.setBpm(140.0);

    EXPECT_DOUBLE_EQ(static_cast<double>(tree[ids::bpm]), 140.0);
    EXPECT_EQ(project.tree(), tree);
}

TEST_F(ProjectTest, FSO01_AddsRenamesRecoloursAndFindsPatterns)
{
    auto added = project.addPattern("p-2", "Fill", 1);
    added.setName("Fill 2");
    added.setColour("#3B82F6");

    ASSERT_EQ(project.numPatterns(), 2);
    EXPECT_EQ(project.pattern(1).name(), "Fill 2");
    EXPECT_EQ(project.pattern(1).colour(), "#3B82F6");
    EXPECT_EQ(project.findPattern("p-2")->lengthBars(), 1);
    EXPECT_FALSE(project.findPattern("unknown").has_value());
}

TEST_F(ProjectTest, FSO01_NewPatternsGetADefaultColour)
{
    EXPECT_EQ(firstPattern().colour(), "#E8743B");
}

TEST_F(ProjectTest, FPR09_PatternLengthIsAtLeastOneBar)
{
    auto pattern = firstPattern();

    pattern.setLengthBars(4);
    EXPECT_EQ(pattern.lengthBars(), 4);

    pattern.setLengthBars(0);
    EXPECT_EQ(pattern.lengthBars(), 1);
}

TEST_F(ProjectTest, FSO05_RemovingAPatternRemovesItsSongBlocks)
{
    project.addPattern("p-2", "Verse", 2);
    auto song = project.song();
    song.addEntry("id-1", 0);
    song.addEntry("p-2", 2);
    song.addEntry("id-1", 4);

    project.removePattern(0);

    ASSERT_EQ(project.numPatterns(), 1);
    EXPECT_EQ(project.pattern(0).id(), "p-2");
    ASSERT_EQ(song.numEntries(), 1);
    EXPECT_EQ(song.entry(0).patternId(), "p-2");
}

TEST_F(ProjectTest, FPR03_AddsChangesAndRemovesNotes)
{
    auto pattern = firstPattern();
    const NoteData kick{36, 0, 240, 100, NoteOrigin::grid};
    const NoteData snareHit{38, 960, 240, 90, NoteOrigin::live};

    pattern.addNote(kick);
    auto note = pattern.addNote(snareHit);
    note.setSlotNote(40);
    note.setStartTick(1000);
    note.setLengthTicks(120);
    note.setOrigin(NoteOrigin::grid);

    ASSERT_EQ(pattern.numNotes(), 2);
    EXPECT_EQ(pattern.note(0).data(), kick);
    EXPECT_EQ(pattern.note(1).data(), (NoteData{40, 1000, 120, 90, NoteOrigin::grid}));

    pattern.removeNote(0);
    ASSERT_EQ(pattern.numNotes(), 1);
    EXPECT_EQ(pattern.note(0).slotNote(), 40);
}

TEST_F(ProjectTest, FPR07_RemembersWhetherANoteWasPlayedLive)
{
    auto note = firstPattern().addNote({38, 0, 240, 100, NoteOrigin::live});

    EXPECT_EQ(note.origin(), NoteOrigin::live);
}

TEST_F(ProjectTest, FPR10_VelocityIsClampedTo1To127)
{
    auto note = firstPattern().addNote({38, 0, 240, 100, NoteOrigin::grid});

    note.setVelocity(0);
    EXPECT_EQ(note.velocity(), 1);

    note.setVelocity(200);
    EXPECT_EQ(note.velocity(), 127);

    note.setVelocity(64);
    EXPECT_EQ(note.velocity(), 64);
}

TEST_F(ProjectTest, FPR10_NewNotesAreClampedToo)
{
    const auto note = firstPattern().addNote({38, 0, 240, 0, NoteOrigin::grid});

    EXPECT_EQ(note.velocity(), 1);
}

TEST_F(ProjectTest, FSE05_SlotMidiNoteCanBeOverriddenWithoutChangingItsIdentity)
{
    auto slot = snare();

    slot.setMidiNote(40);

    EXPECT_EQ(slot.midiNote(), 40);
    EXPECT_EQ(slot.gmNote(), 38);
    EXPECT_EQ(project.kit().findSlot(38)->midiNote(), 40);
}

TEST_F(ProjectTest, FSE06_ChangesSlotGainPitchAndChokeGroup)
{
    auto slot = snare();

    slot.setGain(0.5);
    slot.setPitch(-3.0);
    slot.setChokeGroup(2);

    EXPECT_DOUBLE_EQ(slot.gain(), 0.5);
    EXPECT_DOUBLE_EQ(slot.pitch(), -3.0);
    EXPECT_EQ(slot.chokeGroup(), 2);
}

TEST_F(ProjectTest, FPJ03_SlotWithFileHasASample)
{
    auto slot = snare();

    slot.setFilePath("/samples/snare.wav");

    EXPECT_EQ(slot.filePath(), "/samples/snare.wav");
    EXPECT_TRUE(slot.hasSample());
    EXPECT_FALSE(slot.isSampleMissing());
}

TEST_F(ProjectTest, FPJ03_SlotWithMissingFileHasNoSample)
{
    auto slot = snare();
    slot.setFilePath("/samples/snare.wav");

    tree.getChildWithName(ids::kit).getChild(3).setProperty(ids::sampleMissing, true, nullptr);

    EXPECT_TRUE(slot.isSampleMissing());
    EXPECT_FALSE(slot.hasSample());
    EXPECT_EQ(slot.filePath(), "/samples/snare.wav");
}

TEST_F(ProjectTest, FPJ03_AssigningANewFileClearsTheMissingMarker)
{
    auto slot = snare();
    tree.getChildWithName(ids::kit).getChild(3).setProperty(ids::sampleMissing, true, nullptr);

    slot.setFilePath("/samples/other.wav");

    EXPECT_FALSE(slot.isSampleMissing());
    EXPECT_TRUE(slot.hasSample());
}

TEST_F(ProjectTest, FSE04_FindSlotReturnsNothingForUnknownNote)
{
    EXPECT_FALSE(project.kit().findSlot(80).has_value());
}

TEST_F(ProjectTest, FSO04_SongBlocksReferencePatternsById)
{
    auto song = project.song();

    auto block = song.addEntry("id-1", 0);
    song.addEntry("id-1", 2);
    block.setStartBar(1);

    ASSERT_EQ(song.numEntries(), 2);
    EXPECT_EQ(song.entry(0).patternId(), "id-1");
    EXPECT_EQ(song.entry(0).startBar(), 1);
    EXPECT_EQ(song.entry(1).startBar(), 2);

    song.removeEntry(0);
    ASSERT_EQ(song.numEntries(), 1);
    EXPECT_EQ(song.entry(0).startBar(), 2);
}

TEST_F(ProjectTest, FBT05_ChangesBackingTrack)
{
    auto backing = project.backingTrack();

    backing.setFilePath("/audio/song.wav");
    backing.setOffsetSamples(48000LL * 60 * 30);
    backing.setGain(0.8);

    EXPECT_EQ(backing.filePath(), "/audio/song.wav");
    EXPECT_EQ(backing.offsetSamples(), 86400000LL);
    EXPECT_DOUBLE_EQ(backing.gain(), 0.8);
}

TEST_F(ProjectTest, FBT06_ChangesTheThreeMixGains)
{
    auto mix = project.mix();

    mix.setBackingGain(0.25);
    mix.setDrumsGain(0.5);
    mix.setMasterGain(0.75);

    EXPECT_DOUBLE_EQ(mix.backingGain(), 0.25);
    EXPECT_DOUBLE_EQ(mix.drumsGain(), 0.5);
    EXPECT_DOUBLE_EQ(mix.masterGain(), 0.75);
}

TEST_F(ProjectTest, FPJ05_UndoAndRedoAnEdit)
{
    undoManager.beginNewTransaction();
    project.setBpm(90.0);

    ASSERT_TRUE(undoManager.undo());
    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);

    ASSERT_TRUE(undoManager.redo());
    EXPECT_DOUBLE_EQ(project.bpm(), 90.0);
}

TEST_F(ProjectTest, FPJ05_UndoesARecordingPassAsOneStep)
{
    auto pattern = firstPattern();

    undoManager.beginNewTransaction("Aufnahme");
    pattern.addNote({36, 0, 240, 100, NoteOrigin::live});
    pattern.addNote({38, 960, 240, 100, NoteOrigin::live});
    pattern.addNote({42, 480, 240, 100, NoteOrigin::live});

    ASSERT_TRUE(undoManager.undo());
    EXPECT_EQ(pattern.numNotes(), 0);
}

TEST_F(ProjectTest, FPJ05_UndoRestoresARemovedPatternWithItsSongBlocks)
{
    project.song().addEntry("id-1", 0);

    undoManager.beginNewTransaction();
    project.removePattern(0);
    ASSERT_TRUE(undoManager.undo());

    EXPECT_EQ(project.numPatterns(), 1);
    EXPECT_EQ(project.song().numEntries(), 1);
}

TEST_F(ProjectTest, FPJ02_ReadsValuesStoredAsTextLikeAfterLoading)
{
    tree.setProperty(ids::bpm, "133.5", nullptr);
    tree.setProperty(ids::timeSigNumerator, "6", nullptr);
    project.backingTrack().setOffsetSamples(0);
    tree.getChildWithName(ids::backingTrack).setProperty(ids::offsetSamples, "5000000000", nullptr);

    EXPECT_DOUBLE_EQ(project.bpm(), 133.5);
    EXPECT_EQ(project.timeSignature().numerator, 6);
    EXPECT_EQ(project.backingTrack().offsetSamples(), 5000000000LL);
}

} // namespace
} // namespace drumprog::model
