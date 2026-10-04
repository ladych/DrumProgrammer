#include "engine/TempoMath.h"

namespace drumprog::engine
{
namespace
{

constexpr double kSecondsPerMinute = 60.0;
constexpr int kQuartersPerWholeNote = 4;

} // namespace

double samplesPerTick(double bpm, int ticksPerQuarter, double sampleRate) noexcept
{
    return sampleRate * kSecondsPerMinute / (bpm * ticksPerQuarter);
}

std::int64_t ticksPerBeat(int ticksPerQuarter, int denominator) noexcept
{
    return std::int64_t{ticksPerQuarter} * kQuartersPerWholeNote / denominator;
}

std::int64_t ticksPerBar(int ticksPerQuarter, int numerator, int denominator) noexcept
{
    return ticksPerBeat(ticksPerQuarter, denominator) * numerator;
}

std::int64_t wrapTick(std::int64_t tick, std::int64_t length) noexcept
{
    const std::int64_t wrapped = tick % length;
    return wrapped < 0 ? wrapped + length : wrapped;
}

std::int64_t floorDiv(std::int64_t value, std::int64_t divisor) noexcept
{
    return (value - wrapTick(value, divisor)) / divisor;
}

} // namespace drumprog::engine
