#pragma once

#include "engine/IAudioFileStream.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

namespace drumprog::engine
{

/// Plays the backing track on the audio thread (F-BT-02, F-BT-03): streams it from the disk and
/// resamples it to the device rate with 4-point cubic Hermite interpolation while it plays.
///
/// The caller passes the song position of every block in device samples; the player derives the file
/// position from it anew for every block, so the track can never drift from the drums (Q-06). The offset
/// is the file frame that plays at the song start (F-BT-05): positive skips a lead-in, negative starts the
/// track later.
///
/// The GUI thread publishes a new stream through an atomic pointer; the replaced one is deleted on the
/// GUI thread by collectGarbage(), never in the callback (Q-04).
class BackingTrackPlayer
{
public:
    /// File frames read per step; bounds the memory of the audio thread.
    static constexpr int kReadFrames = 4096;
    /// File rate / device rate beyond this is not played, e.g. 384 kHz on 8 kHz.
    static constexpr double kMaxRatio = 48.0;

    BackingTrackPlayer() = default;
    ~BackingTrackPlayer();

    BackingTrackPlayer(const BackingTrackPlayer&) = delete;
    BackingTrackPlayer& operator=(const BackingTrackPlayer&) = delete;
    BackingTrackPlayer(BackingTrackPlayer&&) = delete;
    BackingTrackPlayer& operator=(BackingTrackPlayer&&) = delete;

    // GUI thread
    /// Replaces the track; null removes it.
    void setStream(std::unique_ptr<IAudioFileStream> stream);
    /// Deletes a stream the audio thread no longer uses. Call regularly, e.g. from a GUI timer.
    void collectGarbage();

    // Audio thread
    void prepare(double sampleRate) noexcept;
    /// Takes over a new stream; call at the start of each block.
    void update() noexcept;
    /// Device samples from the song start to the end of the track, 0 without a track.
    [[nodiscard]] std::int64_t endSample(std::int64_t offsetSamples) const noexcept;
    /// Adds numSamples samples of the track times gain to the outputs; songSample is the song position of
    /// the first one in device samples.
    void render(float* const* outputs,
                int numOutputs,
                int numSamples,
                std::int64_t songSample,
                std::int64_t offsetSamples,
                float gain) noexcept;

private:
    struct Track
    {
        std::unique_ptr<IAudioFileStream> stream;
    };

    /// Where a step of render() goes.
    struct Target
    {
        float* const* outputs = nullptr;
        int numOutputs = 0;
        int offset = 0; ///< first sample in the outputs
        int numSamples = 0;
    };

    /// File rate / device rate, 0 without a track.
    [[nodiscard]] double ratio() const noexcept;
    /// At most kReadFrames - 4 file frames.
    void renderStep(const Target& target, double filePosition, double rate, float gain) noexcept;

    std::atomic<Track*> pending_{nullptr};
    std::atomic<Track*> retired_{nullptr};
    Track* current_ = nullptr; ///< audio thread only
    double sampleRate_ = 48000.0;
    std::array<float, kReadFrames> left_{};
    std::array<float, kReadFrames> right_{};
};

} // namespace drumprog::engine
