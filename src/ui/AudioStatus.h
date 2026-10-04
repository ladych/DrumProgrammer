#pragma once

#include <string>
#include <vector>

namespace drumprog::ui
{

/// What the status bar shows about the audio and MIDI setup (Q-02, F-AO-05).
struct AudioDeviceInfo
{
    bool open = false;
    std::string driver;
    std::string device;
    double sampleRate = 0.0;
    int bufferSize = 0;
    int outputLatencySamples = 0; ///< as reported by the driver, 0 if unknown
    std::vector<std::string> midiInputs;
};

/// Output latency in ms: the driver's value, or one buffer if the driver reports none.
[[nodiscard]] double outputLatencyMs(const AudioDeviceInfo& info);
/// Longest time from a key press to the sound in ms: a hit waits up to one buffer for the next block,
/// then the output latency (Q-01).
[[nodiscard]] double hitToSoundLatencyMs(const AudioDeviceInfo& info);
/// Status line, e.g. "JACK | system | 48000 Hz | Buffer 128 (2,7 ms) | Latenz 5,3 ms, Anschlag bis
/// Ton max. 8,0 ms | MIDI: Nitro".
[[nodiscard]] std::string audioStatusText(const AudioDeviceInfo& info);

} // namespace drumprog::ui
