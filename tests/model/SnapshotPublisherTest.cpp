#include "model/SnapshotPublisher.h"
#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace drumprog::model
{
namespace
{

class SnapshotPublisherTest : public ::testing::Test
{
protected:
    FakeIdGenerator ids;
    juce::ValueTree tree = ProjectFactory{ids}.createDefault();
    Project project{tree, nullptr};
    ProjectSnapshotExchange exchange;
    SnapshotPublisher publisher{tree, exchange};
};

TEST_F(SnapshotPublisherTest, Q04_PublishesTheProjectRightAway)
{
    const auto* snapshot = exchange.acquire();

    ASSERT_NE(snapshot, nullptr);
    EXPECT_DOUBLE_EQ(snapshot->bpm, 120.0);
}

TEST_F(SnapshotPublisherTest, Q04_PublishesAgainAfterEveryChange)
{
    project.setBpm(150.0);
    EXPECT_DOUBLE_EQ(exchange.acquire()->bpm, 150.0);

    project.pattern(0).addNote({36, 0, 240, 100, NoteOrigin::grid});
    EXPECT_EQ(exchange.acquire()->patterns[0].notes.size(), 1U);
}

TEST_F(SnapshotPublisherTest, Q04_OldSnapshotsAreFreedOnThePublishingThread)
{
    for (int bpm = 100; bpm < 110; ++bpm)
        project.setBpm(bpm);

    EXPECT_LE(exchange.numOwned(), 2U);
}

} // namespace
} // namespace drumprog::model
