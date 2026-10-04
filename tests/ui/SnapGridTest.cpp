#include "ui/SnapGrid.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

constexpr int kPpq = 960;

TEST(SnapGridTest, FPR04_StepsOfStraightAndTripletGrids)
{
    EXPECT_EQ(gridStepTicks(GridDivision::quarter, kPpq), 960);
    EXPECT_EQ(gridStepTicks(GridDivision::eighth, kPpq), 480);
    EXPECT_EQ(gridStepTicks(GridDivision::sixteenth, kPpq), 240);
    EXPECT_EQ(gridStepTicks(GridDivision::thirtySecond, kPpq), 120);
    EXPECT_EQ(gridStepTicks(GridDivision::quarterTriplet, kPpq), 640);
    EXPECT_EQ(gridStepTicks(GridDivision::eighthTriplet, kPpq), 320);
    EXPECT_EQ(gridStepTicks(GridDivision::sixteenthTriplet, kPpq), 160);
    EXPECT_EQ(gridStepTicks(GridDivision::thirtySecondTriplet, kPpq), 80);
}

TEST(SnapGridTest, FPR04_LabelsForTheDropdown)
{
    std::string labels;
    for (const auto division : kGridDivisions)
        labels += gridLabel(division) + " ";
    EXPECT_EQ(labels, "1/4 1/8 1/16 1/32 1/4T 1/8T 1/16T 1/32T ");
}

TEST(SnapGridTest, FPR04_SnapsToTheNearestGridLine)
{
    EXPECT_EQ(snapToGrid(0, 240), 0);
    EXPECT_EQ(snapToGrid(119, 240), 0);
    EXPECT_EQ(snapToGrid(120, 240), 240);
    EXPECT_EQ(snapToGrid(350, 240), 240);
    EXPECT_EQ(snapToGrid(-100, 240), 0);
    EXPECT_EQ(snapToGrid(-130, 240), -240);
}

TEST(SnapGridTest, FPR04_FloorsToTheGridLineBefore)
{
    EXPECT_EQ(floorToGrid(479, 240), 240);
    EXPECT_EQ(floorToGrid(480, 240), 480);
    EXPECT_EQ(floorToGrid(-1, 240), -240);
}

TEST(SnapGridTest, FPR04_NamesNoteValues)
{
    EXPECT_EQ(noteValueLabel(3840, kPpq), "1/1");
    EXPECT_EQ(noteValueLabel(1920, kPpq), "1/2");
    EXPECT_EQ(noteValueLabel(240, kPpq), "1/16");
    EXPECT_EQ(noteValueLabel(160, kPpq), "1/16T");
    EXPECT_FALSE(noteValueLabel(250, kPpq).has_value());
}

} // namespace
} // namespace drumprog::ui
