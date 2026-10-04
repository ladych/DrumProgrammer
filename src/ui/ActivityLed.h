#pragma once

#include <cstdint>

namespace drumprog::ui
{

/// One LED that lights up when an activity counter changed and stays on for a few GUI timer ticks,
/// so even a single short hit is visible (F-SE-09, F-IN-05). The counter is written by another
/// thread; the GUI timer passes its current value to update().
class ActivityLed
{
public:
    static constexpr int kHoldTicks = 4; ///< about 130 ms at a 30 Hz GUI timer

    void update(std::uint32_t count) noexcept;
    [[nodiscard]] bool isOn() const noexcept { return ticksLeft_ > 0; }

private:
    std::uint32_t seenCount_ = 0;
    int ticksLeft_ = 0;
};

} // namespace drumprog::ui
