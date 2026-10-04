#include "engine/Clock.h"

#include <gtest/gtest.h>

namespace drumprog::engine
{
namespace
{

TEST(ClockTest, FIN08_SteadyClockNeverGoesBack)
{
    const Clock clock = &steadyClockSeconds;
    const double first = clock();
    EXPECT_GE(clock(), first);
}

} // namespace
} // namespace drumprog::engine
