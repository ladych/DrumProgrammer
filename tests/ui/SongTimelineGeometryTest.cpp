#include "ui/SongTimelineGeometry.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

class SongTimelineGeometryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        geometry.setViewWidth(480); // 10 bars at the default zoom
        geometry.setSongLength(20);
    }

    SongTimelineGeometry geometry;
};

TEST_F(SongTimelineGeometryTest, FSO02_ConvertsBetweenBarsAndPixels)
{
    EXPECT_DOUBLE_EQ(geometry.pixelsPerBar(), 48.0);
    EXPECT_DOUBLE_EQ(geometry.xOfBar(0), 0.0);
    EXPECT_DOUBLE_EQ(geometry.xOfBar(2.5), 120.0);
    EXPECT_EQ(geometry.barAt(0.0), 0);
    EXPECT_EQ(geometry.barAt(47.9), 0);
    EXPECT_EQ(geometry.barAt(48.0), 1);
    EXPECT_EQ(geometry.barAt(-30.0), 0);
    EXPECT_EQ(geometry.firstVisibleBar(), 0);
    EXPECT_EQ(geometry.lastVisibleBar(), 10);
}

TEST_F(SongTimelineGeometryTest, FSO02_ScrollsInsideTheSongPlusRoomForNewBlocks)
{
    geometry.scrollBy(96.0);
    EXPECT_DOUBLE_EQ(geometry.scrollX(), 96.0);
    EXPECT_DOUBLE_EQ(geometry.xOfBar(2), 0.0);
    EXPECT_EQ(geometry.barAt(10.0), 2);
    EXPECT_EQ(geometry.firstVisibleBar(), 2);

    geometry.scrollBy(10000.0);
    EXPECT_DOUBLE_EQ(geometry.scrollX(), (20 + 16) * 48.0 - 480.0);
    geometry.scrollBy(-10000.0);
    EXPECT_DOUBLE_EQ(geometry.scrollX(), 0.0);
}

TEST_F(SongTimelineGeometryTest, FBT04_ExactBarIncludesTheFractionAndNegativeBars)
{
    EXPECT_DOUBLE_EQ(geometry.exactBarAt(60.0), 1.25);
    EXPECT_DOUBLE_EQ(geometry.exactBarAt(-24.0), -0.5);
    geometry.scrollBy(96.0);
    EXPECT_DOUBLE_EQ(geometry.exactBarAt(12.0), 2.25);
}

TEST_F(SongTimelineGeometryTest, FSO02_ShorterSongOrWiderViewPullsTheScrollBack)
{
    geometry.scrollBy(10000.0);
    geometry.setSongLength(0);
    EXPECT_DOUBLE_EQ(geometry.scrollX(), 16 * 48.0 - 480.0);
    geometry.setViewWidth(2000);
    EXPECT_DOUBLE_EQ(geometry.scrollX(), 0.0);
}

TEST_F(SongTimelineGeometryTest, FSO07_ZoomKeepsTheBarAtTheLeftEdge)
{
    geometry.scrollBy(96.0);
    geometry.zoomIn();
    EXPECT_DOUBLE_EQ(geometry.pixelsPerBar(), 72.0);
    EXPECT_DOUBLE_EQ(geometry.xOfBar(2), 0.0);
    geometry.zoomOut();
    EXPECT_DOUBLE_EQ(geometry.pixelsPerBar(), 48.0);
    EXPECT_DOUBLE_EQ(geometry.xOfBar(2), 0.0);
}

TEST_F(SongTimelineGeometryTest, FSO07_ZoomIsLimited)
{
    EXPECT_TRUE(geometry.canZoomIn());
    EXPECT_TRUE(geometry.canZoomOut());
    for (int step = 0; step < 20; ++step)
        geometry.zoomIn();
    EXPECT_DOUBLE_EQ(geometry.pixelsPerBar(), SongTimelineGeometry::kMaxPixelsPerBar);
    EXPECT_FALSE(geometry.canZoomIn());
    for (int step = 0; step < 20; ++step)
        geometry.zoomOut();
    EXPECT_DOUBLE_EQ(geometry.pixelsPerBar(), SongTimelineGeometry::kMinPixelsPerBar);
    EXPECT_FALSE(geometry.canZoomOut());
}

TEST_F(SongTimelineGeometryTest, FSO02_RulerNumbersStayReadableWhenZoomedOut)
{
    EXPECT_EQ(geometry.labelStep(), 1);
    geometry.zoomOut(); // 32 px
    EXPECT_EQ(geometry.labelStep(), 1);
    geometry.zoomOut(); // 21.3 px
    EXPECT_EQ(geometry.labelStep(), 2);
    for (int step = 0; step < 10; ++step)
        geometry.zoomOut(); // 8 px
    EXPECT_EQ(geometry.labelStep(), 4);
}

} // namespace
} // namespace drumprog::ui
