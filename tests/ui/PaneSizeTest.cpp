#include "ui/PaneSize.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

TEST(PaneSizeTest, AP14_ShowsThePreferredSizeWhenThereIsRoom)
{
    const PaneSize pane{260, 160};
    EXPECT_EQ(pane.size(1400, 700), 260);
}

TEST(PaneSizeTest, AP14_LeavesTheOtherPanesTheirMinimum)
{
    const PaneSize pane{260, 160};
    EXPECT_EQ(pane.size(900, 700), 200);
}

TEST(PaneSizeTest, AP14_NeverGetsSmallerThanItsMinimum)
{
    const PaneSize pane{260, 160};
    EXPECT_EQ(pane.size(600, 700), 160);
}

TEST(PaneSizeTest, AP14_ComesBackToThePreferredSizeWhenTheWindowGrowsAgain)
{
    const PaneSize pane{260, 160};
    EXPECT_EQ(pane.size(900, 700), 200);
    EXPECT_EQ(pane.size(1400, 700), 260);
}

TEST(PaneSizeTest, AP14_DraggingChangesTheSizeByTheMouseOffset)
{
    PaneSize pane{260, 160};
    pane.beginDrag(1400, 700);
    pane.dragTo(40, 1400, 700);
    EXPECT_EQ(pane.size(1400, 700), 300);
    pane.dragTo(-30, 1400, 700);
    EXPECT_EQ(pane.size(1400, 700), 230);
    EXPECT_EQ(pane.preferred(), 230);
}

TEST(PaneSizeTest, AP14_DraggingStopsAtTheMinimumAndAtTheRoomOfTheOtherPanes)
{
    PaneSize pane{260, 160};
    pane.beginDrag(1400, 700);
    pane.dragTo(-500, 1400, 700);
    EXPECT_EQ(pane.preferred(), 160);
    pane.dragTo(1000, 1400, 700);
    EXPECT_EQ(pane.preferred(), 700);
}

TEST(PaneSizeTest, AP14_DraggingStartsFromTheShownSizeNotFromALargerWish)
{
    PaneSize pane{260, 160};
    pane.beginDrag(900, 700);
    pane.dragTo(10, 900, 700);
    EXPECT_EQ(pane.preferred(), 200);
}

TEST(PaneSizeTest, AP14_DraggingInAWindowTooSmallForAllPanesKeepsTheMinimum)
{
    PaneSize pane{260, 160};
    pane.beginDrag(600, 700);
    pane.dragTo(50, 600, 700);
    EXPECT_EQ(pane.preferred(), 160);
}

} // namespace
} // namespace drumprog::ui
