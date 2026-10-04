#include "model/TakeRecorder.h"

#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace drumprog::model
{
namespace
{

class TakeRecorderTest : public ::testing::Test
{
protected:
    [[nodiscard]] Pattern pattern() const { return project.pattern(0); }

    FakeIdGenerator ids;
    juce::ValueTree tree = ProjectFactory{ids}.createDefault(); // one pattern of 2 bars = 7680 ticks
    juce::UndoManager undoManager;
    Project project{tree, &undoManager};
    TakeRecorder recorder{tree, undoManager};
};

TEST_F(TakeRecorderTest, FIN07_NeedsAnExistingPattern)
{
    EXPECT_FALSE(recorder.begin(-1, RecordMode::overdub));
    EXPECT_FALSE(recorder.begin(1, RecordMode::overdub));
    EXPECT_FALSE(recorder.isRecording());
    recorder.add(0, 100, 0);
    EXPECT_EQ(pattern().numNotes(), 0);
}

TEST_F(TakeRecorderTest, FIN07_HitsBecomeLiveNotesOfTheirSlot)
{
    ASSERT_TRUE(recorder.begin(0, RecordMode::overdub));
    EXPECT_TRUE(recorder.isRecording());
    recorder.add(2, 90, 1234);

    ASSERT_EQ(pattern().numNotes(), 1);
    const auto note = pattern().note(0).data();
    EXPECT_EQ(note.slotNote, project.kit().slot(2).gmNote());
    EXPECT_EQ(note.startTick, 1234);
    EXPECT_EQ(note.lengthTicks, 240);
    EXPECT_EQ(note.velocity, 90);
    EXPECT_EQ(note.origin, NoteOrigin::live);
}

TEST_F(TakeRecorderTest, FIN10_NotesAreFoldedAndCutAtThePatternEnd)
{
    recorder.begin(0, RecordMode::overdub);
    recorder.add(0, 100, 7600);
    recorder.add(0, 100, 7690);
    EXPECT_EQ(pattern().note(0).lengthTicks(), 80);
    EXPECT_EQ(pattern().note(1).startTick(), 10);
}

TEST_F(TakeRecorderTest, FIN07_IgnoresUnknownSlotsAndARemovedPattern)
{
    recorder.begin(0, RecordMode::overdub);
    recorder.add(-1, 100, 0);
    recorder.add(project.kit().numSlots(), 100, 0);
    EXPECT_EQ(pattern().numNotes(), 0);

    project.addPattern("other", "Other", 1);
    project.removePattern(0);
    recorder.add(0, 100, 0);
    EXPECT_EQ(project.pattern(0).numNotes(), 0);
}

TEST_F(TakeRecorderTest, FIN09_OverdubKeepsAndReplaceRemovesTheNotes)
{
    pattern().addNote({.slotNote = 36, .startTick = 0});
    recorder.begin(0, RecordMode::overdub);
    recorder.add(0, 100, 960);
    recorder.end();
    EXPECT_EQ(pattern().numNotes(), 2);

    recorder.begin(0, RecordMode::replace);
    EXPECT_EQ(pattern().numNotes(), 0);
    recorder.add(0, 100, 480);
    recorder.end();
    ASSERT_EQ(pattern().numNotes(), 1);
    EXPECT_EQ(pattern().note(0).startTick(), 480);
}

TEST_F(TakeRecorderTest, FIN09_FPJ05_TheWholeRunIsOneUndoStep)
{
    pattern().addNote({.slotNote = 36, .startTick = 0});
    undoManager.beginNewTransaction();
    recorder.begin(0, RecordMode::replace);
    recorder.add(0, 100, 100);
    recorder.add(1, 100, 200);
    recorder.end();
    EXPECT_FALSE(recorder.isRecording());
    project.setBpm(90.0);

    ASSERT_TRUE(undoManager.undo());
    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);
    EXPECT_EQ(pattern().numNotes(), 2);
    ASSERT_TRUE(undoManager.undo());
    ASSERT_EQ(pattern().numNotes(), 1);
    EXPECT_EQ(pattern().note(0).startTick(), 0);
}

TEST_F(TakeRecorderTest, FIN09_EditsDuringTheRunStayTheirOwnUndoStep)
{
    recorder.begin(0, RecordMode::overdub);
    recorder.add(0, 100, 100);
    undoManager.beginNewTransaction("Kit");
    project.kit().slot(0).setGain(0.5);
    recorder.add(0, 100, 200);

    ASSERT_TRUE(undoManager.undo());
    EXPECT_EQ(pattern().numNotes(), 1);
    EXPECT_DOUBLE_EQ(project.kit().slot(0).gain(), 0.5);
}

TEST_F(TakeRecorderTest, FIN09_EndWithoutRunDoesNothing)
{
    recorder.end();
    EXPECT_FALSE(recorder.isRecording());
}

} // namespace
} // namespace drumprog::model
