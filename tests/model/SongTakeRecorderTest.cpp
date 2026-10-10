#include "model/SongTakeRecorder.h"

#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace drumprog::model
{
namespace
{

constexpr std::int64_t kBar = 3840;

class SongTakeRecorderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // "Pattern 1" (id-1, 2 bars) on bar 1.
        project.song().addEntry("id-1", 0);
        undoManager.clearUndoHistory();
    }

    [[nodiscard]] Kit kit() const { return project.activeKit(globalKit); }

    FakeIdGenerator ids;
    juce::ValueTree tree = ProjectFactory{ids}.createDefault();
    juce::UndoManager undoManager;
    Project project{tree, &undoManager};
    juce::ValueTree globalKit = ProjectFactory::createDefaultKit();
    SongTakeRecorder recorder{tree, globalKit, undoManager, ids};
};

TEST_F(SongTakeRecorderTest, FBT08_ATakeBecomesANewPatternWithABlockWhereItStarted)
{
    recorder.begin();
    EXPECT_TRUE(recorder.isRecording());
    recorder.add(1, 90, 8 * kBar + 100);
    recorder.add(3, 110, 9 * kBar + 960);
    const auto index = recorder.end(8 * kBar, 9 * kBar + 2000);
    EXPECT_FALSE(recorder.isRecording());

    ASSERT_EQ(index, 1);
    const auto take = project.pattern(1);
    EXPECT_EQ(take.name(), "Take 1");
    EXPECT_EQ(take.lengthBars(), 2);
    ASSERT_EQ(take.numNotes(), 2);
    const auto first = take.note(0).data();
    EXPECT_EQ(first.slotNote, kit().slot(1).gmNote());
    EXPECT_EQ(first.startTick, 100);
    EXPECT_EQ(first.lengthTicks, 240);
    EXPECT_EQ(first.velocity, 90);
    EXPECT_EQ(first.origin, NoteOrigin::live);
    EXPECT_EQ(take.note(1).data().startTick, kBar + 960);

    ASSERT_EQ(project.song().numEntries(), 2);
    EXPECT_EQ(project.song().entry(1).patternId(), take.id());
    EXPECT_EQ(project.song().entry(1).startBar(), 8);
}

TEST_F(SongTakeRecorderTest, FBT08_BarsAreRoundedToWholeBarsAroundStartStopAndHits)
{
    recorder.begin();
    recorder.add(1, 100, 2 * kBar + 500);
    recorder.add(1, 100, 4 * kBar + 10); // after the stop, still recorded
    ASSERT_TRUE(recorder.end(2 * kBar + 300, 3 * kBar + 1).has_value());
    const auto take = project.pattern(1);
    EXPECT_EQ(take.lengthBars(), 3);
    EXPECT_EQ(take.note(0).data().startTick, 500);
    EXPECT_EQ(project.song().entry(1).startBar(), 2);
}

TEST_F(SongTakeRecorderTest, FBT08_EarlyHitsGoToTheStartAndNotesAreCutAtTheEnd)
{
    recorder.begin();
    recorder.add(1, 100, kBar - 20);    // 1/16 before the start
    recorder.add(2, 100, 2 * kBar - 1); // last tick
    ASSERT_TRUE(recorder.end(kBar, kBar + 10).has_value());
    const auto take = project.pattern(1);
    EXPECT_EQ(take.lengthBars(), 1);
    EXPECT_EQ(take.note(0).data().startTick, 0);
    EXPECT_EQ(take.note(1).data().startTick, kBar - 1);
    EXPECT_EQ(take.note(1).data().lengthTicks, 1);
}

TEST_F(SongTakeRecorderTest, FBT08_ATakeWithoutHitsAddsNothing)
{
    recorder.begin();
    EXPECT_FALSE(recorder.end(0, 4 * kBar).has_value());
    EXPECT_FALSE(recorder.end(0, 4 * kBar).has_value());
    EXPECT_EQ(project.numPatterns(), 1);
    EXPECT_FALSE(undoManager.canUndo());
}

TEST_F(SongTakeRecorderTest, FBT07_HitsWithoutTakeOrForUnknownSlotsAreIgnored)
{
    recorder.add(1, 100, 0);
    recorder.begin();
    recorder.add(-1, 100, 0);
    recorder.add(kit().numSlots(), 100, 0);
    EXPECT_FALSE(recorder.end(0, kBar).has_value());

    recorder.add(1, 100, 0); // after the end
    recorder.begin();
    EXPECT_FALSE(recorder.end(0, kBar).has_value());
}

TEST_F(SongTakeRecorderTest, FBT08_TakesAreNumberedAndAreOneUndoStepEach)
{
    project.addPattern("x", "Take 4", 1);
    project.addPattern("y", "Take", 1);
    project.addPattern("z", "Take zwei", 1);
    undoManager.clearUndoHistory();
    const auto before = project.pattern(0).tree().createCopy();

    recorder.begin();
    recorder.add(1, 100, 0);
    recorder.end(0, 10);
    EXPECT_EQ(project.pattern(4).name(), "Take 5");
    EXPECT_TRUE(project.pattern(0).tree().isEquivalentTo(before)); // existing patterns stay
    recorder.begin();
    recorder.add(1, 100, 0);
    recorder.end(-50, 10);
    EXPECT_EQ(project.pattern(5).name(), "Take 6");
    EXPECT_EQ(project.song().entry(2).startBar(), 0);

    undoManager.undo();
    EXPECT_EQ(project.numPatterns(), 5);
    EXPECT_EQ(project.song().numEntries(), 2);
    undoManager.undo();
    EXPECT_EQ(project.numPatterns(), 4);
    EXPECT_EQ(project.song().numEntries(), 1);
}

} // namespace
} // namespace drumprog::model
