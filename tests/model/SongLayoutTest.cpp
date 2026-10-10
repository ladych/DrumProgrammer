#include "model/SongLayout.h"

#include "model/FakeIdGenerator.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace drumprog::model
{
namespace
{

constexpr std::int64_t kBar = 3840;

class SongLayoutTest : public ::testing::Test
{
protected:
    void SetUp() override { project.addPattern("fill", "Fill", 1); }

    FakeIdGenerator ids;
    Project project{ProjectFactory{ids}.createDefault(), nullptr}; // "Pattern 1" (id-1), 2 bars
    Song song = project.song();
};

TEST_F(SongLayoutTest, FSO04_BlocksAreSortedByStartAndResolvedToTheirPattern)
{
    song.addEntry("fill", 2);
    song.addEntry("id-1", 0);

    const auto blocks = layoutSong(project);

    ASSERT_EQ(blocks.size(), 2U);
    EXPECT_EQ(blocks[0].entryIndex, 1);
    EXPECT_EQ(blocks[0].patternIndex, 0);
    EXPECT_EQ(blocks[0].startTick, 0);
    EXPECT_EQ(blocks[0].lengthTicks, 2 * kBar);
    EXPECT_EQ(blocks[0].playedTicks, 2 * kBar);
    EXPECT_EQ(blocks[1].entryIndex, 0);
    EXPECT_EQ(blocks[1].patternIndex, 1);
    EXPECT_EQ(blocks[1].startTick, 2 * kBar);
    EXPECT_EQ(blocks[1].playedTicks, kBar);
    EXPECT_EQ(songLengthTicks(blocks), 3 * kBar);
}

TEST_F(SongLayoutTest, FSO04_BlocksOfUnknownPatternsAreLeftOut)
{
    song.addEntry("deleted", 0);
    song.addEntry("fill", 4);

    const auto blocks = layoutSong(project);

    ASSERT_EQ(blocks.size(), 1U);
    EXPECT_EQ(blocks[0].entryIndex, 1);
}

TEST_F(SongLayoutTest, FTR05_ANextBlockCutsTheBlockBefore)
{
    song.addEntry("id-1", 0);
    song.addEntry("fill", 1);

    const auto blocks = layoutSong(project);

    EXPECT_EQ(blocks[0].lengthTicks, 2 * kBar);
    EXPECT_EQ(blocks[0].playedTicks, kBar);
    EXPECT_EQ(songLengthTicks(blocks), 2 * kBar);
}

TEST_F(SongLayoutTest, FTR05_OfBlocksOnTheSameBarTheOneAddedLastPlays)
{
    song.addEntry("id-1", 3);
    song.addEntry("fill", 3);

    const auto blocks = layoutSong(project);

    ASSERT_EQ(blocks.size(), 2U);
    EXPECT_EQ(blocks[0].patternIndex, 0);
    EXPECT_EQ(blocks[0].playedTicks, 0);
    EXPECT_EQ(blocks[1].patternIndex, 1);
    EXPECT_EQ(blocks[1].playedTicks, kBar);
    EXPECT_EQ(songLengthTicks(blocks), 4 * kBar);
}

TEST_F(SongLayoutTest, FTR05_EmptySongHasNoLength)
{
    EXPECT_TRUE(layoutSong(project).empty());
    EXPECT_EQ(songLengthTicks({}), 0);
}

TEST_F(SongLayoutTest, FSO04_BlocksFollowTheLengthOfTheirPattern)
{
    song.addEntry("fill", 0);
    project.pattern(1).setLengthBars(4);

    EXPECT_EQ(songLengthTicks(layoutSong(project)), 4 * kBar);
}

} // namespace
} // namespace drumprog::model
