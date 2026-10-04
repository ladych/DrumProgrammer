#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace drumprog::engine
{

/// Hit counters per kit slot for the trigger LEDs (F-SE-09). The audio thread
/// increments, a GUI timer reads and compares with the last value it saw.
class TriggerIndicators
{
public:
    static constexpr int kMaxSlots = 128;

    /// Ignores slot indices outside 0..kMaxSlots-1.
    void signal(int slotIndex) noexcept;
    /// Returns 0 for slot indices outside 0..kMaxSlots-1.
    [[nodiscard]] std::uint32_t hitCount(int slotIndex) const noexcept;

private:
    std::array<std::atomic<std::uint32_t>, kMaxSlots> hits_{};
};

} // namespace drumprog::engine
