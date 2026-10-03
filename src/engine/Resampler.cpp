#include "engine/Resampler.h"

#include <cmath>
#include <cstddef>

namespace drumprog::engine
{
namespace
{

/// Frames outside the sample count as silence.
float frameAt(const std::vector<float>& channel, long long index) noexcept
{
    if (index < 0 || index >= static_cast<long long>(channel.size()))
        return 0.0F;
    return channel[static_cast<std::size_t>(index)];
}

float hermite(const std::vector<float>& channel, double position) noexcept
{
    const auto index = static_cast<long long>(std::floor(position));
    const auto t = static_cast<float>(position - static_cast<double>(index));
    const float xm1 = frameAt(channel, index - 1);
    const float x0 = frameAt(channel, index);
    const float x1 = frameAt(channel, index + 1);
    const float x2 = frameAt(channel, index + 2);
    const float c1 = 0.5F * (x1 - xm1);
    const float c2 = xm1 - 2.5F * x0 + 2.0F * x1 - 0.5F * x2;
    const float c3 = 0.5F * (x2 - xm1) + 1.5F * (x0 - x1);
    return ((c3 * t + c2) * t + c1) * t + x0;
}

std::vector<float> resampleChannel(const std::vector<float>& source, double step, std::size_t frames)
{
    std::vector<float> result(frames);
    for (std::size_t i = 0; i < frames; ++i)
        result[i] = hermite(source, static_cast<double>(i) * step);
    return result;
}

} // namespace

SampleBuffer resample(const SampleBuffer& source, double targetSampleRate)
{
    if (source.sampleRate <= 0.0 || targetSampleRate <= 0.0 || source.sampleRate == targetSampleRate)
        return source;

    const double step = source.sampleRate / targetSampleRate;
    const auto frames = static_cast<std::size_t>(std::llround(source.numFrames() / step));
    SampleBuffer result{targetSampleRate, {}};
    result.channels.reserve(source.channels.size());
    for (const auto& channel : source.channels)
        result.channels.push_back(resampleChannel(channel, step, frames));
    return result;
}

} // namespace drumprog::engine
