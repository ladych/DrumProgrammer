#pragma once

namespace drumprog::engine
{

/// Dynamic range covered by velocity 1..127.
inline constexpr float kVelocityRangeDb = 40.0F;

/// Maps MIDI velocity linearly onto a dB scale (F-SE-06): 127 is 0 dB, 1 is
/// -40 dB. Velocity 0 or below is silence, larger values are treated as 127.
[[nodiscard]] float velocityToGain(int velocity) noexcept;

} // namespace drumprog::engine
