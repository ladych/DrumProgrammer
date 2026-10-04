#include "input/MidiInputHandler.h"

namespace drumprog::input
{
namespace
{

constexpr std::uint8_t kStatusMask = 0xF0;
constexpr std::uint8_t kNoteOn = 0x90;
constexpr std::uint8_t kFirstRealtimeStatus = 0xF8; ///< clock, start/stop, active sensing, reset
constexpr std::size_t kNoteOnSize = 3;

} // namespace

MidiInputHandler::MidiInputHandler(INoteSink& sink, InputActivity& activity)
    : sink_(sink), activity_(activity)
{
}

void MidiInputHandler::handleMessage(std::span<const std::uint8_t> message) noexcept
{
    if (message.empty() || message[0] >= kFirstRealtimeStatus)
        return;
    activity_.signalMidi();
    if ((message[0] & kStatusMask) != kNoteOn || message.size() < kNoteOnSize)
        return;
    const int velocity = message[2];
    if (velocity > 0)
        sink_.noteOn(message[1], velocity);
}

} // namespace drumprog::input
