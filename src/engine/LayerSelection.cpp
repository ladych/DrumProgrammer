#include "engine/LayerSelection.h"

#include <algorithm>
#include <cstddef>

namespace drumprog::engine
{
namespace
{

constexpr int kMaxVelocity = 127;

int nearestLayer(std::span<const LayerSample> layers, float target) noexcept
{
    const auto above = std::ranges::lower_bound(layers, target, {}, &LayerSample::power);
    if (above == layers.end())
        return static_cast<int>(layers.size()) - 1;
    const auto index = static_cast<int>(above - layers.begin());
    if (index == 0)
        return 0;
    const float below = layers[static_cast<std::size_t>(index - 1)].power;
    return target - below <= above->power - target ? index - 1 : index;
}

} // namespace

int selectLayer(std::span<const LayerSample> layers,
                int velocity,
                std::uint32_t random,
                int lastIndex) noexcept
{
    if (layers.empty())
        return -1;
    const float target = static_cast<float>(std::clamp(velocity, 1, kMaxVelocity)) / kMaxVelocity;
    const int nearest = nearestLayer(layers, target);
    const int first = std::max(nearest - 1, 0);
    const int last = std::min(nearest + 1, static_cast<int>(layers.size()) - 1);
    const int count = last - first + 1;
    if (count == 1)
        return first;
    if (lastIndex < first || lastIndex > last)
        return first + static_cast<int>(random % static_cast<std::uint32_t>(count));
    const int pick = first + static_cast<int>(random % static_cast<std::uint32_t>(count - 1));
    return pick >= lastIndex ? pick + 1 : pick;
}

std::uint32_t FastRandom::next() noexcept
{
    state_ ^= state_ << 13U;
    state_ ^= state_ >> 17U;
    state_ ^= state_ << 5U;
    return state_;
}

} // namespace drumprog::engine
