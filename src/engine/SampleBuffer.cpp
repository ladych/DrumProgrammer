#include "engine/SampleBuffer.h"

#include <algorithm>

namespace drumprog::engine
{

int SampleBuffer::numChannels() const noexcept
{
    return static_cast<int>(channels.size());
}

int SampleBuffer::numFrames() const noexcept
{
    if (channels.empty())
        return 0;
    const auto shortest = std::min_element(
        channels.begin(), channels.end(), [](const auto& a, const auto& b) { return a.size() < b.size(); });
    return static_cast<int>(shortest->size());
}

} // namespace drumprog::engine
