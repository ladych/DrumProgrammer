#include "engine/VelocityCurve.h"

#include <gtest/gtest.h>

#include <cmath>

namespace drumprog::engine
{
namespace
{

float toDb(float gain)
{
    return 20.0F * std::log10(gain);
}

TEST(VelocityCurveTest, FSE06_FullVelocityIsUnityGain)
{
    EXPECT_FLOAT_EQ(velocityToGain(127), 1.0F);
}

TEST(VelocityCurveTest, FSE06_LowestVelocityIsBottomOfRange)
{
    EXPECT_NEAR(toDb(velocityToGain(1)), -kVelocityRangeDb, 1.0e-4);
}

TEST(VelocityCurveTest, FSE06_VelocityIsLinearInDecibels)
{
    const float step = toDb(velocityToGain(101)) - toDb(velocityToGain(100));
    EXPECT_NEAR(step, kVelocityRangeDb / 126.0F, 1.0e-4);
    EXPECT_NEAR(toDb(velocityToGain(64)), -kVelocityRangeDb / 2.0F, 1.0e-4);
}

TEST(VelocityCurveTest, FSE06_ClampsOutOfRangeVelocity)
{
    EXPECT_FLOAT_EQ(velocityToGain(0), 0.0F);
    EXPECT_FLOAT_EQ(velocityToGain(-5), 0.0F);
    EXPECT_FLOAT_EQ(velocityToGain(200), 1.0F);
}

} // namespace
} // namespace drumprog::engine
