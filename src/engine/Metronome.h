#pragma once

#include <array>
#include <atomic>

namespace drumprog::engine
{

/// Click of the metronome and the count-in (F-TR-08, F-TR-09): a short sine burst that fades out,
/// higher on the first beat of a bar. The level is set from the GUI thread, everything else runs on
/// the audio thread without allocation (Q-04).
class Metronome
{
public:
    static constexpr double kClickHz = 1000.0;
    static constexpr double kAccentHz = 1600.0;
    static constexpr double kClickSeconds = 0.03;
    static constexpr float kDefaultLevel = 0.5F;
    static constexpr int kMaxClicksPerBlock = 8;

    // GUI thread
    /// Linear gain 0..1.
    void setLevel(float level) noexcept;
    [[nodiscard]] float level() const noexcept;

    // Audio thread
    void prepare(double sampleRate) noexcept;
    /// Starts a click at a sample position of the next render() call; false if the block is full.
    bool click(bool accent, int sampleOffset) noexcept;
    /// Adds the clicks to all outputs.
    void render(float* const* outputs, int numOutputs, int numSamples) noexcept;

private:
    struct PendingClick
    {
        bool accent = false;
        int sampleOffset = 0;
    };

    void start(bool accent) noexcept;
    [[nodiscard]] float nextSample() noexcept;

    std::atomic<float> level_{kDefaultLevel};
    double sampleRate_ = 48000.0;
    std::array<PendingClick, kMaxClicksPerBlock> pending_{};
    int numPending_ = 0;
    int length_ = 0;
    int remaining_ = 0;
    double phase_ = 0.0;
    double phaseIncrement_ = 0.0;
};

} // namespace drumprog::engine
