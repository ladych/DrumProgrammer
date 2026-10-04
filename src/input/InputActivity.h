#pragma once

#include <atomic>
#include <cstdint>

namespace drumprog::input
{

/// Counters for the input LEDs "MIDI In" and "Tastatur" (F-IN-05). The MIDI thread and the GUI
/// thread increment, a GUI timer reads; lock-free.
class InputActivity
{
public:
    void signalMidi() noexcept;
    void signalKeyboard() noexcept;
    [[nodiscard]] std::uint32_t midiCount() const noexcept;
    [[nodiscard]] std::uint32_t keyboardCount() const noexcept;

private:
    std::atomic<std::uint32_t> midi_{0};
    std::atomic<std::uint32_t> keyboard_{0};
};

} // namespace drumprog::input
