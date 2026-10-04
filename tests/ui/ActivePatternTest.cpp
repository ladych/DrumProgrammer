#include "ui/ActivePattern.h"

#include "model/FakeIdGenerator.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

#include <vector>

namespace drumprog::ui
{
namespace
{

class ActivePatternTest : public ::testing::Test
{
protected:
    ActivePatternTest()
    {
        project.addPattern("p-2", "Verse", 2);
        project.addPattern("p-3", "Fill", 1);
        active.addOnChange([this](int index) { changes.push_back(index); });
    }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    model::Project project{tree, nullptr};
    ActivePattern active{tree};
    std::vector<int> changes;
};

TEST_F(ActivePatternTest, FSO01_StartsWithTheFirstPattern)
{
    EXPECT_EQ(active.index(), 0);
}

TEST_F(ActivePatternTest, FSO01_SelectsPatternsInsideTheList)
{
    active.select(2);
    active.select(2);
    active.select(3);
    active.select(-1);

    EXPECT_EQ(active.index(), 2);
    EXPECT_EQ(changes, std::vector<int>{2});
}

TEST_F(ActivePatternTest, FSO01_FollowsItsPatternWhenOthersAreRemoved)
{
    active.select(2);
    project.removePattern(0);

    EXPECT_EQ(active.index(), 1);
    EXPECT_EQ(changes, (std::vector<int>{2, 1}));
}

TEST_F(ActivePatternTest, FSO01_TakesThePatternAtItsPlaceWhenItIsRemoved)
{
    active.select(1);
    project.removePattern(1);
    EXPECT_EQ(active.index(), 1);
    EXPECT_EQ(project.pattern(active.index()).id(), "p-3");

    project.removePattern(1);
    EXPECT_EQ(active.index(), 0);
}

TEST_F(ActivePatternTest, FSO01_IsMinusOneWithoutPatterns)
{
    tree.getChildWithName(model::ids::patterns).removeAllChildren(nullptr);

    EXPECT_EQ(active.index(), -1);

    project.addPattern("p-4", "Neu", 2);
    EXPECT_EQ(active.index(), 0);
}

TEST_F(ActivePatternTest, FPJ01_FallsBackToTheFirstPatternOfALoadedProject)
{
    active.select(2);
    tree.copyPropertiesAndChildrenFrom(model::ProjectFactory{ids}.createDefault(), nullptr);

    EXPECT_EQ(active.index(), 0);
}

} // namespace
} // namespace drumprog::ui
