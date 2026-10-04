#pragma once

#include "input/INoteSink.h"
#include "input/InputActivity.h"

#include <cstdint>
#include <span>

namespace drumprog::input
{

/// MIDI drum kit as trigger (F-IN-03, F-IN-04, F-IN-05). Runs on the MIDI thread: no allocation and
/// no lock, the sink queues the hit lock-free for the audio thread.
class MidiInputHandler
{
public:
    MidiInputHandler(INoteSink& sink, InputActivity& activity);

    /// One raw MIDI message. A note-on with velocity > 0 on any channel triggers its note. Note-off
    /// and note-on with velocity 0 do nothing, because drum samples are one-shots. Every message
    /// except clock and active sensing lights the "MIDI In" LED.
    void handleMessage(std::span<const std::uint8_t> message) noexcept;

private:
    INoteSink& sink_;
    InputActivity& activity_;
};

} // namespace drumprog::input
