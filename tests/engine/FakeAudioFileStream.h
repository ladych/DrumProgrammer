#pragma once

#include "engine/IAudioFileStream.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace drumprog::engine
{

/// In-memory backing track; frames outside the file are silence like on the disk.
class FakeAudioFileStream final : public IAudioFileStream
{
public:
    FakeAudioFileStream(double sampleRate, std::vector<float> left, std::vector<float> right)
        : sampleRate_(sampleRate), left_(std::move(left)), right_(std::move(right))
    {
    }

    /// Frame n has the value n + 1 on the left and -(n + 1) on the right.
    static std::unique_ptr<FakeAudioFileStream> ramp(double sampleRate, int frames)
    {
        std::vector<float> left;
        std::vector<float> right;
        for (int frame = 0; frame < frames; ++frame)
        {
            left.push_back(static_cast<float>(frame + 1));
            right.push_back(-static_cast<float>(frame + 1));
        }
        return std::make_unique<FakeAudioFileStream>(sampleRate, std::move(left), std::move(right));
    }

    [[nodiscard]] double sampleRate() const noexcept override { return sampleRate_; }
    [[nodiscard]] std::int64_t lengthInSamples() const noexcept override
    {
        return static_cast<std::int64_t>(left_.size());
    }

    void read(std::int64_t start, int numFrames, float* left, float* right) noexcept override
    {
        largestRead = std::max(largestRead, numFrames);
        for (int index = 0; index < numFrames; ++index)
        {
            const std::int64_t frame = start + index;
            const bool inside = frame >= 0 && frame < lengthInSamples();
            left[index] = inside ? left_[static_cast<std::size_t>(frame)] : 0.0F;
            right[index] = inside ? right_[static_cast<std::size_t>(frame)] : 0.0F;
        }
    }

    int largestRead = 0;

private:
    double sampleRate_;
    std::vector<float> left_;
    std::vector<float> right_;
};

} // namespace drumprog::engine
