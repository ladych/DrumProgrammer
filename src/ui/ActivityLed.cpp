#include "ui/ActivityLed.h"

namespace drumprog::ui
{

void ActivityLed::update(std::uint32_t count) noexcept
{
    if (count != seenCount_)
    {
        seenCount_ = count;
        ticksLeft_ = kHoldTicks;
    }
    else if (ticksLeft_ > 0)
    {
        --ticksLeft_;
    }
}

} // namespace drumprog::ui
