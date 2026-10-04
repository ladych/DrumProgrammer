#include "model/SnapshotPublisher.h"
#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

#include <vector>

namespace drumprog::model
{
namespace
{

class FakeKitSink : public engine::IKitSink
{
public:
    void publishKit(const engine::KitDescription& kit) override { kits.push_back(kit); }

    std::vector<engine::KitDescription> kits;
};

class SnapshotPublisherTest : public ::testing::Test
{
protected:
    FakeIdGenerator ids;
    juce::ValueTree tree = ProjectFactory{ids}.createDefault();
    Project project{tree, nullptr};
    juce::ValueTree globalKit = ProjectFactory::createDefaultKit();
    ProjectSnapshotExchange exchange;
    FakeKitSink kitSink;
    SnapshotPublisher publisher{tree, globalKit, exchange, kitSink};
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

TEST_F(SnapshotPublisherTest, FSE04_HandsTheKitToTheEngineRightAway)
{
    ASSERT_EQ(kitSink.kits.size(), 1U);
    EXPECT_EQ(kitSink.kits.back().size(), 25U);
}

TEST_F(SnapshotPublisherTest, FSE05_HandsTheKitOverAgainWhenASlotOfTheGlobalKitChanges)
{
    Kit{globalKit, nullptr}.findSlot(38)->setMidiNote(40);

    ASSERT_EQ(kitSink.kits.size(), 2U);
    EXPECT_EQ(kitSink.kits.back()[3].midiNote, 40);
}

TEST_F(SnapshotPublisherTest, FSE05_TheProjectsOwnKitOverridesTheGlobalKit)
{
    auto ownKit = ProjectFactory::createDefaultKit();
    Kit{ownKit, nullptr}.findSlot(36)->setFilePath("/kits/own-kick.wav");
    project.setOwnKit(ownKit);
    EXPECT_EQ(kitSink.kits.back()[1].sampleFile, std::filesystem::path{"/kits/own-kick.wav"});

    project.kit().findSlot(38)->setMidiNote(40);
    EXPECT_EQ(kitSink.kits.back()[3].midiNote, 40);

    project.removeOwnKit();
    EXPECT_TRUE(kitSink.kits.back()[1].sampleFile.empty());
}

TEST_F(SnapshotPublisherTest, FPJ02_ALoadedProjectWithItsOwnKitReplacesTheKit)
{
    auto loaded = ProjectFactory{ids}.createDefault();
    Project loadedProject{loaded, nullptr};
    loadedProject.setOwnKit(ProjectFactory::createDefaultKit());
    loadedProject.kit().findSlot(36)->setFilePath("/kits/kick.wav");

    tree.copyPropertiesAndChildrenFrom(loaded, nullptr);

    EXPECT_EQ(kitSink.kits.back()[1].sampleFile, std::filesystem::path{"/kits/kick.wav"});
}

TEST_F(SnapshotPublisherTest, Q04_OldSnapshotsAreFreedOnThePublishingThread)
{
    for (int bpm = 100; bpm < 110; ++bpm)
        project.setBpm(bpm);

    EXPECT_LE(exchange.numOwned(), 2U);
}

} // namespace
} // namespace drumprog::model
