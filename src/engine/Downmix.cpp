#include "engine/Downmix.h"

#include <cmath>
#include <cstddef>

namespace drumprog::engine
{

SampleBuffer downmixToStereo(const SampleBuffer& source)
{
    if (source.numChannels() <= 2)
        return source;

    const auto frames = static_cast<std::size_t>(source.numFrames());
    // Equal gain per microphone; the sqrt keeps the level of correlated mics in range.
    const float gain = 1.0F / std::sqrt(static_cast<float>(source.numChannels()));
    std::vector<float> mix(frames, 0.0F);
    for (const auto& channel : source.channels)
        for (std::size_t i = 0; i < frames; ++i)
            mix[i] += gain * channel[i];
    SampleBuffer result{source.sampleRate, {}};
    result.channels.assign(2, mix);
    return result;
}

} // namespace drumprog::engine
