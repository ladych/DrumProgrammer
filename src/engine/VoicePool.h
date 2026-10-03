#pragma once

#include "engine/Voice.h"

#include <array>
#include <cstdint>

namespace drumprog::engine
{

/// Polyphonic playback with a fixed number of voices (F-SE-03). When all 64
/// voices are playing, the oldest one is faded out and the new note takes one
/// of the reserve voices, so stealing never clicks. Audio thread only.
class VoicePool
{
public:
    static constexpr int kMaxVoices = 64;
    static constexpr int kReserveVoices = 8;
    static constexpr double kFadeSeconds = 0.005; ///< steal and choke fade (F-SE-08)

    /// Not real-time safe to call concurrently with render(); call before playback.
    void prepare(double sampleRate) noexcept;

    /// Starts a voice. A choke group > 0 first fades out every voice of that group.
    void trigger(const VoiceStart& params) noexcept;
    void choke(int chokeGroup) noexcept;
    void stopAll() noexcept;

    void render(float* const* outputs, int numOutputs, int numSamples) noexcept;

    /// Voices that are playing and not fading out; at most kMaxVoices.
    [[nodiscard]] int playingVoiceCount() const noexcept;
    /// All voices producing sound, including fading ones.
    [[nodiscard]] int soundingVoiceCount() const noexcept;
    [[nodiscard]] bool isOwnerInUse(const void* owner) const noexcept;
    [[nodiscard]] int fadeSamples() const noexcept;

private:
    [[nodiscard]] Voice* oldestVoice(bool releasing) noexcept;
    [[nodiscard]] Voice& freeVoice() noexcept;

    std::array<Voice, kMaxVoices + kReserveVoices> voices_{};
    std::uint64_t nextStartOrder_ = 0;
    int fadeSamples_ = 1;
};

} // namespace drumprog::engine
