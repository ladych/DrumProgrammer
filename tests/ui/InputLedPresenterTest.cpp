#include "ui/InputLedPresenter.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

TEST(InputLedPresenterTest, FIN05_MidiAndKeyboardLedsBlinkSeparately)
{
    input::InputActivity activity;
    InputLedPresenter leds{activity};

    leds.tick();
    EXPECT_FALSE(leds.isMidiLedOn());
    EXPECT_FALSE(leds.isKeyboardLedOn());

    activity.signalMidi();
    leds.tick();
    EXPECT_TRUE(leds.isMidiLedOn());
    EXPECT_FALSE(leds.isKeyboardLedOn());

    activity.signalKeyboard();
    for (int tick = 0; tick < ActivityLed::kHoldTicks; ++tick)
        leds.tick();
    EXPECT_FALSE(leds.isMidiLedOn());
    EXPECT_TRUE(leds.isKeyboardLedOn());
}

} // namespace
} // namespace drumprog::ui
