#include "input/InputActivity.h"

namespace drumprog::input
{

void InputActivity::signalMidi() noexcept
{
    midi_.fetch_add(1, std::memory_order_relaxed);
}

void InputActivity::signalKeyboard() noexcept
{
    keyboard_.fetch_add(1, std::memory_order_relaxed);
}

std::uint32_t InputActivity::midiCount() const noexcept
{
    return midi_.load(std::memory_order_relaxed);
}

std::uint32_t InputActivity::keyboardCount() const noexcept
{
    return keyboard_.load(std::memory_order_relaxed);
}

} // namespace drumprog::input
