#include "ui/AudioStatus.h"

#include "i18n/Translate.h"

#include <cmath>
#include <string>

namespace drumprog::ui
{
namespace
{

constexpr double kMsPerSecond = 1000.0;

/// One decimal with the decimal separator of the language.
std::string formatMs(double milliseconds)
{
    const long long tenths = std::llround(milliseconds * 10.0);
    return i18n::tr("%1 ms",
                    std::to_string(tenths / 10) + i18n::decimalSeparator() + std::to_string(tenths % 10));
}

std::string midiText(const std::vector<std::string>& inputs)
{
    if (inputs.empty())
        return i18n::tr("MIDI: no input");
    std::string text;
    for (std::size_t index = 0; index < inputs.size(); ++index)
        text += (index > 0 ? ", " : "") + inputs[index];
    return i18n::tr("MIDI: %1", text);
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
    std::string warning;
    if (info.stalled)
        warning = i18n::tr("Audio device not responding, retrying | ");
    if (!info.open || info.sampleRate <= 0.0)
        return warning + i18n::tr("No audio device open | ") + midiText(info.midiInputs);
    std::string replaced;
    if (!info.replacedDevice.empty())
        replaced = i18n::tr("Replacing %1 | ", info.replacedDevice);
    std::string xruns;
    if (info.xruns >= 0)
        xruns = i18n::tr("XRuns: %1 | ", std::to_string(info.xruns));
    const double bufferMs = info.bufferSize * kMsPerSecond / info.sampleRate;
    return warning + replaced + info.driver + " | " + info.device + " | " +
           i18n::tr("%1 Hz | Buffer %2 (%3) | Latency %4, hit to sound max. %5 | ",
                    std::to_string(std::llround(info.sampleRate)),
                    std::to_string(info.bufferSize),
                    formatMs(bufferMs),
                    formatMs(outputLatencyMs(info)),
                    formatMs(hitToSoundLatencyMs(info))) +
           xruns + midiText(info.midiInputs);
}

} // namespace drumprog::ui
