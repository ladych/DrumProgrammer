#pragma once

#include "engine/EngineKit.h"

#include <cstdint>
#include <span>

namespace drumprog::engine
{

/// Picks the hit of a DrumGizmo instrument for a velocity (F-DG-02): the hit whose power is nearest to
/// velocity/127 and its neighbours form the candidates, one of them is taken at random, but never the one
/// played last time if there is another (no machine-gun effect). Real-time safe (Q-04).
/// Returns -1 for an empty list.
[[nodiscard]] int
selectLayer(std::span<const LayerSample> layers, int velocity, std::uint32_t random, int lastIndex) noexcept;

/// Small xorshift generator for the audio thread: no allocation, no locks.
class FastRandom
{
public:
    [[nodiscard]] std::uint32_t next() noexcept;

private:
    std::uint32_t state_ = 0x9E3779B9U;
};

} // namespace drumprog::engine
