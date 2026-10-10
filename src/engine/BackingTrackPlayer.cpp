#include "engine/BackingTrackPlayer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace drumprog::engine
{
namespace
{

/// Frames around a position the interpolation needs: one before, two after.
constexpr int kExtraFrames = 4;

float hermite(const float* frames, float fraction) noexcept
{
    const float before = frames[-1];
    const float current = frames[0];
    const float next = frames[1];
    const float after = frames[2];
    const float c1 = 0.5F * (next - before);
    const float c2 = before - 2.5F * current + 2.0F * next - 0.5F * after;
    const float c3 = 0.5F * (after - before) + 1.5F * (current - next);
    return ((c3 * fraction + c2) * fraction + c1) * fraction + current;
}

} // namespace

BackingTrackPlayer::~BackingTrackPlayer()
{
    delete pending_.load();
    delete retired_.load();
    delete current_;
}

void BackingTrackPlayer::setStream(std::unique_ptr<IAudioFileStream> stream)
{
    // A track the audio thread never picked up is still owned by this thread.
    delete pending_.exchange(new Track{std::move(stream)}, std::memory_order_acq_rel);
}

void BackingTrackPlayer::collectGarbage()
{
    delete retired_.exchange(nullptr, std::memory_order_acq_rel);
}

void BackingTrackPlayer::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
}

void BackingTrackPlayer::update() noexcept
{
    // The GUI thread clears the retire slot; until it does, the current track stays.
    if (retired_.load(std::memory_order_acquire) != nullptr)
        return;
    if (Track* next = pending_.exchange(nullptr, std::memory_order_acq_rel))
    {
        retired_.store(current_, std::memory_order_release);
        current_ = next;
    }
}

std::int64_t BackingTrackPlayer::endSample(std::int64_t offsetSamples) const noexcept
{
    const double rate = ratio();
    if (rate <= 0.0)
        return 0;
    const auto frames = static_cast<double>(current_->stream->lengthInSamples() - offsetSamples);
    return std::max<std::int64_t>(0, static_cast<std::int64_t>(std::ceil(frames / rate)));
}

void BackingTrackPlayer::render(float* const* outputs,
                                int numOutputs,
                                int numSamples,
                                std::int64_t songSample,
                                std::int64_t offsetSamples,
                                float gain) noexcept
{
    const double rate = ratio();
    if (rate <= 0.0 || rate > kMaxRatio || numOutputs <= 0)
        return;
    const int maxStep = static_cast<int>((kReadFrames - kExtraFrames) / rate);
    for (int done = 0; done < numSamples;)
    {
        const int step = std::min(numSamples - done, maxStep);
        // Every step starts from the absolute song position, so rounding errors never add up.
        const double filePosition =
            static_cast<double>(songSample + done) * rate + static_cast<double>(offsetSamples);
        renderStep({outputs, numOutputs, done, step}, filePosition, rate, gain);
        done += step;
    }
}

double BackingTrackPlayer::ratio() const noexcept
{
    if (current_ == nullptr || current_->stream == nullptr)
        return 0.0;
    return current_->stream->sampleRate() / sampleRate_;
}

void BackingTrackPlayer::renderStep(const Target& target,
                                    double filePosition,
                                    double rate,
                                    float gain) noexcept
{
    const auto first = static_cast<std::int64_t>(std::floor(filePosition)) - 1;
    const auto last =
        static_cast<std::int64_t>(std::floor(filePosition + (target.numSamples - 1) * rate)) + 2;
    current_->stream->read(first, static_cast<int>(last - first + 1), left_.data(), right_.data());
    float* const* outputs = target.outputs;
    for (int index = 0; index < target.numSamples; ++index)
    {
        const double position = filePosition + index * rate - static_cast<double>(first);
        const auto frame = static_cast<std::size_t>(position);
        const auto fraction = static_cast<float>(position - static_cast<double>(frame));
        const float left = hermite(&left_.at(frame), fraction) * gain;
        const float right = hermite(&right_.at(frame), fraction) * gain;
        const int sample = target.offset + index;
        if (target.numOutputs == 1)
            outputs[0][sample] += 0.5F * (left + right);
        else
        {
            outputs[0][sample] += left;
            outputs[1][sample] += right;
        }
    }
}

} // namespace drumprog::engine
