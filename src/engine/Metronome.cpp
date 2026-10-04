#include "engine/Metronome.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace drumprog::engine
{

void Metronome::setLevel(float level) noexcept
{
    level_.store(std::clamp(level, 0.0F, 1.0F));
}

float Metronome::level() const noexcept
{
    return level_.load();
}

void Metronome::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
    length_ = static_cast<int>(std::lround(kClickSeconds * sampleRate));
    remaining_ = 0;
    numPending_ = 0;
}

bool Metronome::click(bool accent, int sampleOffset) noexcept
{
    if (numPending_ == kMaxClicksPerBlock)
        return false;
    pending_.at(static_cast<std::size_t>(numPending_++)) = {accent, sampleOffset};
    return true;
}

void Metronome::render(float* const* outputs, int numOutputs, int numSamples) noexcept
{
    const float level = level_.load();
    int next = 0;
    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (next < numPending_ && pending_.at(static_cast<std::size_t>(next)).sampleOffset == sample)
            start(pending_.at(static_cast<std::size_t>(next++)).accent);
        if (remaining_ == 0)
            continue;
        const float value = level * nextSample();
        for (int channel = 0; channel < numOutputs; ++channel)
            outputs[channel][sample] += value;
    }
    numPending_ = 0;
}

void Metronome::start(bool accent) noexcept
{
    remaining_ = length_;
    phase_ = 0.0;
    phaseIncrement_ = 2.0 * std::numbers::pi * (accent ? kAccentHz : kClickHz) / sampleRate_;
}

float Metronome::nextSample() noexcept
{
    const double envelope = static_cast<double>(remaining_) / length_;
    const double value = envelope * std::sin(phase_);
    phase_ += phaseIncrement_;
    --remaining_;
    return static_cast<float>(value);
}

} // namespace drumprog::engine
