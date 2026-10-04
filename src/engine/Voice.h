#pragma once

#include "engine/SampleBuffer.h"

#include <cstdint>

namespace drumprog::engine
{

struct VoiceStart
{
    const SampleBuffer* sample = nullptr;
    float gain = 1.0F;
    double playbackRate = 1.0;
    int chokeGroup = 0;
    int startOffset = 0;         ///< first sample in the current block (sample-accurate start)
    const void* owner = nullptr; ///< kit the sample belongs to, see VoicePool::isOwnerInUse
};

/// Plays one sample once (one-shot). Runs on the audio thread: no allocation,
/// no locks, no virtual calls (Q-04, E-07).
class Voice
{
public:
    void start(const VoiceStart& params, std::uint64_t startOrder) noexcept;
    /// Fades out linearly over fadeSamples and then stops. Keeps a shorter running fade.
    void release(int fadeSamples) noexcept;
    void stop() noexcept;

    [[nodiscard]] bool isActive() const noexcept;
    [[nodiscard]] bool isReleasing() const noexcept;
    [[nodiscard]] int chokeGroup() const noexcept;
    [[nodiscard]] std::uint64_t startOrder() const noexcept;
    [[nodiscard]] const void* owner() const noexcept;

    /// Adds the voice to the output. Mono samples go to every channel; a sample
    /// with fewer channels than the output repeats its last channel.
    void render(float* const* outputs, int numOutputs, int numSamples) noexcept;

private:
    [[nodiscard]] float valueAt(int channel, int index, float fraction) const noexcept;
    [[nodiscard]] float nextEnvelope() noexcept;
    void writeFrame(float* const* outputs, int numOutputs, int outputIndex, float gain) const noexcept;

    VoiceStart params_;
    std::uint64_t startOrder_ = 0;
    bool active_ = false;
    double position_ = 0.0;
    int pendingOffset_ = 0;
    int fadeRemaining_ = 0;
    float fadeStep_ = 0.0F;
    float fadeGain_ = 1.0F;
};

} // namespace drumprog::engine
