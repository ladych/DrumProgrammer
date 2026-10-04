#pragma once

#include <cstdint>

namespace drumprog::engine
{

/// Conversions between musical ticks and audio samples (Pflichtenheft chapter 4, F-TR-06).

/// Length of one tick in samples; fractional, so positions are computed from it, never summed up.
[[nodiscard]] double samplesPerTick(double bpm, int ticksPerQuarter, double sampleRate) noexcept;
/// One beat is one note value of the denominator, e.g. an eighth in 6/8.
[[nodiscard]] std::int64_t ticksPerBeat(int ticksPerQuarter, int denominator) noexcept;
[[nodiscard]] std::int64_t ticksPerBar(int ticksPerQuarter, int numerator, int denominator) noexcept;
/// Position inside a loop of the given length (> 0), also for negative ticks.
[[nodiscard]] std::int64_t wrapTick(std::int64_t tick, std::int64_t length) noexcept;
/// Rounds towards minus infinity, also for negative values.
[[nodiscard]] std::int64_t floorDiv(std::int64_t value, std::int64_t divisor) noexcept;

} // namespace drumprog::engine
