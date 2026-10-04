#pragma once

#include "input/MidiInputHandler.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace drumprog::app
{

/// Humble object (E-03): receives the messages of all enabled MIDI inputs on the MIDI thread and
/// forwards the raw bytes to input::MidiInputHandler (F-IN-03).
class MidiInputCallback final : public juce::MidiInputCallback
{
public:
    explicit MidiInputCallback(input::MidiInputHandler& handler) : handler_(handler) {}

    void handleIncomingMidiMessage(juce::MidiInput* /*source*/, const juce::MidiMessage& message) override
    {
        handler_.handleMessage(std::span<const std::uint8_t>{
            message.getRawData(), static_cast<std::size_t>(message.getRawDataSize())});
    }

private:
    input::MidiInputHandler& handler_;
};

} // namespace drumprog::app
