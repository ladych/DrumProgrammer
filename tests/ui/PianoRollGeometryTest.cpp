#include "ui/PianoRollGeometry.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

constexpr std::int64_t kBar = 3840;

class PianoRollGeometryTest : public ::testing::Test
{
protected:
    PianoRollGeometryTest()
    {
        geometry.setViewSize(800, 240);
        geometry.setContent(4 * kBar, 16); // 4 bars = 1536 px at 96 px per quarter, 16 rows = 384 px
    }

    PianoRollGeometry geometry{960};
};

TEST_F(PianoRollGeometryTest, FPR08_ConvertsBetweenPixelsAndTicksAndRows)
{
    EXPECT_DOUBLE_EQ(geometry.xOf(960), 96.0);
    EXPECT_EQ(geometry.tickAt(96.0), 960);
    EXPECT_EQ(geometry.tickAt(95.9), 959);
    EXPECT_EQ(geometry.ticksFor(24.0), 240);
    EXPECT_DOUBLE_EQ(geometry.yOf(2), 48.0);
    EXPECT_EQ(geometry.rowAt(47.0), 1);
    EXPECT_EQ(geometry.rowAt(-1.0), -1);
}

TEST_F(PianoRollGeometryTest, FPR08_ScrollsInsideTheContent)
{
    geometry.scrollBy(100.0, 50.0);
    EXPECT_DOUBLE_EQ(geometry.scrollX(), 100.0);
    EXPECT_DOUBLE_EQ(geometry.scrollY(), 50.0);
    EXPECT_EQ(geometry.tickAt(0.0), 1000);
    EXPECT_EQ(geometry.rowAt(0.0), 2);

    geometry.scrollBy(10000.0, 10000.0);
    EXPECT_DOUBLE_EQ(geometry.scrollX(), 1536.0 - 800.0);
    EXPECT_DOUBLE_EQ(geometry.scrollY(), 384.0 - 240.0);

    geometry.scrollBy(-10000.0, -10000.0);
    EXPECT_DOUBLE_EQ(geometry.scrollX(), 0.0);
    EXPECT_DOUBLE_EQ(geometry.scrollY(), 0.0);
}

TEST_F(PianoRollGeometryTest, FPR08_ContentSmallerThanTheViewDoesNotScroll)
{
    geometry.setContent(kBar, 4);
    geometry.scrollBy(50.0, 50.0);
    EXPECT_DOUBLE_EQ(geometry.scrollX(), 0.0);
    EXPECT_DOUBLE_EQ(geometry.scrollY(), 0.0);

    geometry.setViewSize(-1, -1);
    EXPECT_EQ(geometry.lastVisibleTick(), 0);
}

TEST_F(PianoRollGeometryTest, FPR08_ZoomsHorizontallyAroundTheAnchor)
{
    geometry.scrollBy(200.0, 0.0);
    const auto anchorTick = geometry.tickAt(300.0);

    geometry.zoomHorizontally(2.0, 300.0);

    EXPECT_DOUBLE_EQ(geometry.pixelsPerQuarter(), 192.0);
    EXPECT_NEAR(static_cast<double>(geometry.tickAt(300.0)), static_cast<double>(anchorTick), 1.0);

    geometry.zoomHorizontally(100.0, 0.0);
    EXPECT_DOUBLE_EQ(geometry.pixelsPerQuarter(), PianoRollGeometry::kMaxPixelsPerQuarter);
    geometry.zoomHorizontally(0.0001, 0.0);
    EXPECT_DOUBLE_EQ(geometry.pixelsPerQuarter(), PianoRollGeometry::kMinPixelsPerQuarter);
}

TEST_F(PianoRollGeometryTest, FPR08_ZoomsVerticallyAroundTheAnchor)
{
    geometry.scrollBy(0.0, 48.0);
    geometry.zoomVertically(1.5, 0.0);

    EXPECT_EQ(geometry.rowHeight(), 36);
    EXPECT_EQ(geometry.rowAt(0.0), 2);

    geometry.zoomVertically(10.0, 0.0);
    EXPECT_EQ(geometry.rowHeight(), PianoRollGeometry::kMaxRowHeight);
    geometry.zoomVertically(0.01, 0.0);
    EXPECT_EQ(geometry.rowHeight(), PianoRollGeometry::kMinRowHeight);
}

TEST_F(PianoRollGeometryTest, FPR08_VisibleRange)
{
    geometry.scrollBy(96.0, 24.0);
    EXPECT_EQ(geometry.firstVisibleTick(), 960);
    EXPECT_EQ(geometry.lastVisibleTick(), 960 + 8000);
    EXPECT_EQ(geometry.firstVisibleRow(), 1);
    EXPECT_EQ(geometry.lastVisibleRow(), 11);

    geometry.setViewSize(800, 2000);
    EXPECT_EQ(geometry.firstVisibleRow(), 0);
    EXPECT_EQ(geometry.lastVisibleRow(), 15);
}

TEST_F(PianoRollGeometryTest, FPR08_BarLinesAreStrongerThanBeatLines)
{
    geometry.setViewSize(400, 240); // ticks 0..4000

    const auto lines = geometry.gridLines(kBar, 960, 240);

    ASSERT_EQ(lines.size(), 17U);
    EXPECT_EQ(lines[0].kind, GridLineKind::bar);
    EXPECT_EQ(lines[1].tick, 240);
    EXPECT_EQ(lines[1].kind, GridLineKind::step);
    EXPECT_EQ(lines[4].kind, GridLineKind::beat);
    EXPECT_EQ(lines[16].tick, kBar);
    EXPECT_EQ(lines[16].kind, GridLineKind::bar);
}

TEST_F(PianoRollGeometryTest, FPR08_TripletStepsKeepTheBeatLines)
{
    geometry.setViewSize(200, 240); // ticks 0..2000

    const auto lines = geometry.gridLines(kBar, 960, 640);

    std::vector<std::int64_t> ticks;
    for (const auto& line : lines)
        ticks.push_back(line.tick);
    EXPECT_EQ(ticks, (std::vector<std::int64_t>{0, 640, 960, 1280, 1920}));
}

TEST_F(PianoRollGeometryTest, FPR08_DenseStepsAreLeftOut)
{
    geometry.zoomHorizontally(0.0001, 0.0); // 12 px per quarter: a 1/32 step is 1.5 px
    geometry.setViewSize(100, 240);
    geometry.scrollBy(10.0, 0.0); // ticks 800..8800

    const auto lines = geometry.gridLines(kBar, 960, 120);

    ASSERT_EQ(lines.size(), 9U);
    EXPECT_EQ(lines.front().tick, 960);
    for (const auto& line : lines)
        EXPECT_NE(line.kind, GridLineKind::step);
    EXPECT_EQ(lines.back().tick, 8640);
}

} // namespace
} // namespace drumprog::ui
