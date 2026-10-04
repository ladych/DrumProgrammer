#include "input/InputActivity.h"

#include <gtest/gtest.h>

#include <thread>

namespace drumprog::input
{
namespace
{

TEST(InputActivityTest, FIN05_CountsMidiAndKeyboardSeparately)
{
    InputActivity activity;
    activity.signalMidi();
    activity.signalMidi();
    activity.signalKeyboard();
    EXPECT_EQ(activity.midiCount(), 2U);
    EXPECT_EQ(activity.keyboardCount(), 1U);
}

TEST(InputActivityTest, FIN05_MidiThreadAndGuiThreadMaySignalConcurrently)
{
    InputActivity activity;
    std::thread midi(
        [&activity]
        {
            for (int i = 0; i < 1000; ++i)
                activity.signalMidi();
        });
    for (int i = 0; i < 1000; ++i)
        static_cast<void>(activity.midiCount());
    midi.join();
    EXPECT_EQ(activity.midiCount(), 1000U);
}

} // namespace
} // namespace drumprog::input
