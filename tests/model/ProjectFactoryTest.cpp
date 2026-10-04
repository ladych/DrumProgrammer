#include "model/ProjectFactory.h"
#include "TestComparisons.h"
#include "engine/KitDescription.h"
#include "model/FakeIdGenerator.h"
#include "model/ModelIds.h"
#include "model/Project.h"

#include <gtest/gtest.h>

namespace drumprog::model
{
namespace
{

class ProjectFactoryTest : public ::testing::Test
{
protected:
    FakeIdGenerator ids;
    ProjectFactory factory{ids};
    juce::ValueTree tree = factory.createDefault();
    Project project{tree, nullptr};
};

TEST_F(ProjectFactoryTest, FPJ01_NewProjectHas120BpmIn4_4)
{
    EXPECT_TRUE(tree.hasType(ids::project));
    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);
    EXPECT_EQ(project.timeSignature(), (TimeSignature{4, 4}));
    EXPECT_EQ(project.ticksPerQuarter(), 960);
    EXPECT_EQ(project.name(), "Neues Projekt");
}

TEST_F(ProjectFactoryTest, FPJ01_NewProjectHasOneEmptyPatternOfTwoBars)
{
    ASSERT_EQ(project.numPatterns(), 1);
    const auto pattern = project.pattern(0);
    EXPECT_EQ(pattern.id(), "id-1");
    EXPECT_EQ(pattern.name(), "Pattern 1");
    EXPECT_EQ(pattern.lengthBars(), 2);
    EXPECT_EQ(pattern.numNotes(), 0);
}

TEST_F(ProjectFactoryTest, FPJ01_NewProjectHasEmptySongAndBackingTrackAndNeutralMix)
{
    EXPECT_EQ(project.song().numEntries(), 0);
    EXPECT_EQ(project.backingTrack().filePath(), "");
    EXPECT_DOUBLE_EQ(project.mix().backingGain(), 1.0);
    EXPECT_DOUBLE_EQ(project.mix().drumsGain(), 1.0);
    EXPECT_DOUBLE_EQ(project.mix().masterGain(), 1.0);
}

TEST_F(ProjectFactoryTest, FPJ01_NewProjectHasNoOwnKitAndUsesTheGlobalKit)
{
    const juce::ValueTree globalKit = ProjectFactory::createDefaultKit();

    EXPECT_FALSE(project.hasOwnKit());
    EXPECT_EQ(project.kit().numSlots(), 0);
    EXPECT_EQ(project.activeKit(globalKit).tree(), globalKit);
}

TEST_F(ProjectFactoryTest, FSE04_DefaultKitCoversGmNotes35To59)
{
    const Kit kit{ProjectFactory::createDefaultKit(), nullptr};

    ASSERT_EQ(kit.numSlots(), 25);
    for (int index = 0; index < kit.numSlots(); ++index)
    {
        const auto slot = kit.slot(index);
        EXPECT_EQ(slot.gmNote(), 35 + index);
        EXPECT_EQ(slot.midiNote(), slot.gmNote());
        EXPECT_FALSE(slot.name().empty());
        EXPECT_FALSE(slot.hasSample());
        EXPECT_DOUBLE_EQ(slot.gain(), 1.0);
        EXPECT_DOUBLE_EQ(slot.pitch(), 0.0);
    }
}

TEST_F(ProjectFactoryTest, FSE04_DefaultKitUsesGmNames)
{
    const Kit kit{ProjectFactory::createDefaultKit(), nullptr};

    EXPECT_EQ(kit.findSlot(36)->name(), "Bass Drum 1");
    EXPECT_EQ(kit.findSlot(38)->name(), "Acoustic Snare");
    EXPECT_EQ(kit.findSlot(42)->name(), "Closed Hi-Hat");
    EXPECT_EQ(kit.findSlot(57)->name(), "Crash Cymbal 2");
}

TEST_F(ProjectFactoryTest, FSE08_HiHatsShareOneChokeGroup)
{
    const Kit kit{ProjectFactory::createDefaultKit(), nullptr};

    EXPECT_EQ(kit.findSlot(42)->chokeGroup(), engine::kHiHatChokeGroup);
    EXPECT_EQ(kit.findSlot(44)->chokeGroup(), engine::kHiHatChokeGroup);
    EXPECT_EQ(kit.findSlot(46)->chokeGroup(), engine::kHiHatChokeGroup);
    EXPECT_EQ(kit.findSlot(38)->chokeGroup(), 0);
}

TEST_F(ProjectFactoryTest, FPJ01_EachNewProjectGetsANewPatternId)
{
    const Project second{factory.createDefault(), nullptr};

    EXPECT_EQ(second.pattern(0).id(), "id-2");
}

} // namespace
} // namespace drumprog::model
