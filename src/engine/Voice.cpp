#include "engine/Voice.h"

#include <algorithm>
#include <cstddef>

namespace drumprog::engine
{

void Voice::start(const VoiceStart& params, std::uint64_t startOrder) noexcept
{
    if (params.sample == nullptr || params.sample->numFrames() == 0)
        return;
    params_ = params;
    startOrder_ = startOrder;
    active_ = true;
    position_ = 0.0;
    pendingOffset_ = std::max(params.startOffset, 0);
    fadeRemaining_ = 0;
    fadeGain_ = 1.0F;
}

void Voice::release(int fadeSamples) noexcept
{
    if (!active_)
        return;
    if (fadeSamples <= 0)
    {
        stop();
        return;
    }
    if (isReleasing() && fadeRemaining_ <= fadeSamples)
        return;
    fadeRemaining_ = fadeSamples;
    fadeStep_ = fadeGain_ / static_cast<float>(fadeSamples);
}

void Voice::stop() noexcept
{
    active_ = false;
    fadeRemaining_ = 0;
}

bool Voice::isActive() const noexcept
{
    return active_;
}

bool Voice::isReleasing() const noexcept
{
    return active_ && fadeRemaining_ > 0;
}

int Voice::chokeGroup() const noexcept
{
    return params_.chokeGroup;
}

std::uint64_t Voice::startOrder() const noexcept
{
    return startOrder_;
}

const void* Voice::owner() const noexcept
{
    return params_.owner;
}

void Voice::render(float* const* outputs, int numOutputs, int numSamples) noexcept
{
    const int first = std::min(pendingOffset_, numSamples);
    pendingOffset_ -= first;
    const int frames = params_.sample != nullptr ? params_.sample->numFrames() : 0;

    for (int i = first; i < numSamples && active_; ++i)
    {
        const auto index = static_cast<int>(position_);
        if (index >= frames)
        {
            stop();
            break;
        }
        const float envelope = nextEnvelope();
        writeFrame(outputs, numOutputs, i, params_.gain * envelope);
        position_ += params_.playbackRate;
    }
}

float Voice::valueAt(int channel, int index, float fraction) const noexcept
{
    const auto& data = params_.sample->channels[static_cast<std::size_t>(channel)];
    const auto frames = static_cast<int>(data.size());
    const float current = data[static_cast<std::size_t>(index)];
    const float next = index + 1 < frames ? data[static_cast<std::size_t>(index) + 1] : 0.0F;
    return current + (next - current) * fraction;
}

float Voice::nextEnvelope() noexcept
{
    if (fadeRemaining_ == 0)
        return 1.0F;
    const float value = fadeGain_;
    fadeGain_ -= fadeStep_;
    if (--fadeRemaining_ == 0)
        active_ = false;
    return value;
}

void Voice::writeFrame(float* const* outputs, int numOutputs, int outputIndex, float gain) const noexcept
{
    const auto index = static_cast<int>(position_);
    const auto fraction = static_cast<float>(position_ - index);
    const int lastChannel = params_.sample->numChannels() - 1;
    for (int channel = 0; channel < numOutputs; ++channel)
    {
        float* const output = outputs[channel];
        if (output != nullptr)
            output[outputIndex] += gain * valueAt(std::min(channel, lastChannel), index, fraction);
    }
}

} // namespace drumprog::engine
