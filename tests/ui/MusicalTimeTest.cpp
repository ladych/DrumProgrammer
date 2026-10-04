#include "ui/MusicalTime.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

TEST(MusicalTimeTest, FPR06_FormatsBarBeatTick)
{
    EXPECT_EQ(formatPosition(0, 960, {4, 4}), "001.1.000");
    EXPECT_EQ(formatPosition(4800, 960, {4, 4}), "002.2.000");
    EXPECT_EQ(formatPosition(3840 + 960 + 90, 960, {4, 4}), "002.2.090");
    EXPECT_EQ(formatPosition(2880 + 480 * 4 + 15, 960, {6, 8}), "002.5.015");
}

} // namespace
} // namespace drumprog::ui
