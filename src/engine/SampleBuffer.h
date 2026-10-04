#pragma once

#include <vector>

namespace drumprog::engine
{

/// Audio data held completely in RAM (F-SE-02), one vector per channel.
struct SampleBuffer
{
    double sampleRate = 0.0;
    std::vector<std::vector<float>> channels;

    [[nodiscard]] int numChannels() const noexcept;
    /// Length of the shortest channel, 0 without channels.
    [[nodiscard]] int numFrames() const noexcept;
};

} // namespace drumprog::engine
