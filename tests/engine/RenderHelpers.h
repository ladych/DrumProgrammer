#pragma once

#include <array>
#include <vector>

namespace drumprog::engine
{

/// Stereo output buffer for render tests, cleared to zero.
struct StereoOutput
{
    explicit StereoOutput(int frames)
        : left(static_cast<size_t>(frames), 0.0F), right(static_cast<size_t>(frames), 0.0F),
          channels{left.data(), right.data()}
    {
    }

    [[nodiscard]] int frames() const noexcept { return static_cast<int>(left.size()); }

    std::vector<float> left;
    std::vector<float> right;
    std::array<float*, 2> channels;
};

} // namespace drumprog::engine
