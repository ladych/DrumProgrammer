#include "ui/ActivityLed.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

TEST(ActivityLedTest, FIN05_IsOffWhileTheCounterDoesNotChange)
{
    ActivityLed led;
    led.update(0);
    EXPECT_FALSE(led.isOn());
}

TEST(ActivityLedTest, FIN05_LightsOnChangeAndGoesOffAfterTheHoldTime)
{
    ActivityLed led;
    led.update(1);
    EXPECT_TRUE(led.isOn());
    for (int tick = 1; tick < ActivityLed::kHoldTicks; ++tick)
        led.update(1);
    EXPECT_TRUE(led.isOn());
    led.update(1);
    EXPECT_FALSE(led.isOn());
}

TEST(ActivityLedTest, FIN05_EveryNewHitRestartsTheHoldTime)
{
    ActivityLed led;
    led.update(1);
    led.update(1);
    led.update(2);
    for (int tick = 1; tick < ActivityLed::kHoldTicks; ++tick)
        led.update(2);
    EXPECT_TRUE(led.isOn());
}

} // namespace
} // namespace drumprog::ui
