#include "ui/AudioStatus.h"

#include <cmath>
#include <string>

namespace drumprog::ui
{
namespace
{

constexpr double kMsPerSecond = 1000.0;

/// One decimal with a German decimal comma.
std::string formatMs(double milliseconds)
{
    const long long tenths = std::llround(milliseconds * 10.0);
    return std::to_string(tenths / 10) + "," + std::to_string(tenths % 10) + " ms";
}

std::string midiText(const std::vector<std::string>& inputs)
{
    if (inputs.empty())
        return "MIDI: kein Eingang";
    std::string text = "MIDI: ";
    for (std::size_t index = 0; index < inputs.size(); ++index)
        text += (index > 0 ? ", " : "") + inputs[index];
    return text;
}

} // namespace

double outputLatencyMs(const AudioDeviceInfo& info)
{
    const int samples = info.outputLatencySamples > 0 ? info.outputLatencySamples : info.bufferSize;
    return samples * kMsPerSecond / info.sampleRate;
}

double hitToSoundLatencyMs(const AudioDeviceInfo& info)
{
    return info.bufferSize * kMsPerSecond / info.sampleRate + outputLatencyMs(info);
}

std::string audioStatusText(const AudioDeviceInfo& info)
{
    if (!info.open || info.sampleRate <= 0.0)
        return "Kein Audio-Gerät geöffnet | " + midiText(info.midiInputs);
    const double bufferMs = info.bufferSize * kMsPerSecond / info.sampleRate;
    return info.driver + " | " + info.device + " | " + std::to_string(std::llround(info.sampleRate)) +
           " Hz | Buffer " + std::to_string(info.bufferSize) + " (" + formatMs(bufferMs) + ") | Latenz " +
           formatMs(outputLatencyMs(info)) + ", Anschlag bis Ton max. " +
           formatMs(hitToSoundLatencyMs(info)) + " | " + midiText(info.midiInputs);
}

} // namespace drumprog::ui
