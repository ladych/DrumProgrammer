#include "engine/TriggerIndicators.h"

#include <cstddef>

namespace drumprog::engine
{
namespace
{

constexpr bool isValidSlot(int slotIndex) noexcept
{
    return slotIndex >= 0 && slotIndex < TriggerIndicators::kMaxSlots;
}

} // namespace

void TriggerIndicators::signal(int slotIndex) noexcept
{
    if (isValidSlot(slotIndex))
        hits_.at(static_cast<std::size_t>(slotIndex)).fetch_add(1, std::memory_order_relaxed);
}

std::uint32_t TriggerIndicators::hitCount(int slotIndex) const noexcept
{
    return isValidSlot(slotIndex)
               ? hits_.at(static_cast<std::size_t>(slotIndex)).load(std::memory_order_relaxed)
               : 0U;
}

} // namespace drumprog::engine
