#include "engine/PlaybackRenderer.h"

#include "engine/RenderHelpers.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace drumprog::engine
{
namespace
{

constexpr double kDeviceRate = 48000.0;
constexpr double kFileRate = 44100.0;
constexpr double kBpm = 120.0;
constexpr int kTicksPerQuarter = 960;
constexpr std::int64_t kTicksPerBar = 4 * kTicksPerQuarter;
/// One bar at 120 BPM lasts 2 seconds.
constexpr std::int64_t kDeviceSamplesPerBar = 96000;
constexpr std::int64_t kFileFramesPerBar = 88200;
constexpr int kBars = 301; // a little more than 10 minutes

/// Backing track with a click at the start of every bar on the left channel only; computed instead of
/// stored, because 10 minutes would not fit into a test's memory.
class BarClickStream final : public IAudioFileStream
{
public:
    [[nodiscard]] double sampleRate() const noexcept override { return kFileRate; }
    [[nodiscard]] std::int64_t lengthInSamples() const noexcept override { return kBars * kFileFramesPerBar; }

    void read(std::int64_t start, int numFrames, float* left, float* right) noexcept override
    {
        for (int index = 0; index < numFrames; ++index)
        {
            const std::int64_t frame = start + index;
            left[index] =
                frame >= 0 && frame < lengthInSamples() && frame % kFileFramesPerBar == 0 ? 1.0F : 0.0F;
            right[index] = 0.0F;
        }
    }
};

/// Q-06 over a long run: the song plays a drum note at the start of every bar, the backing track (at another
/// sample rate) a click. Both have to stay on the same sample for more than 10 minutes, whatever the driver's
/// block sizes are.
class LongRunTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto sample =
            std::make_shared<const SampleBuffer>(SampleBuffer{kDeviceRate, {std::vector<float>(4, 1.0F)}});
        engine.setKit(std::make_unique<EngineKit>(
            std::vector<EngineSlot>{EngineSlot{.slotIndex = 0, .midiNote = 36, .sample = sample}}));
        snapshot.bpm = kBpm;
        snapshot.ticksPerQuarter = kTicksPerQuarter;
        snapshot.patterns.push_back({kTicksPerBar, {NoteSnapshot{0, 0, 240, 127}}});
        for (int bar = 0; bar < kBars; ++bar)
            snapshot.song.push_back({0, bar * kTicksPerBar, kTicksPerBar});
        snapshot.songLengthTicks = kBars * kTicksPerBar;
        backing.setStream(std::make_unique<BarClickStream>());
        renderer.prepare(kDeviceRate);
    }

    ProjectSnapshot snapshot;
    Sequencer sequencer;
    SampleEngine engine;
    Metronome metronome;
    BackingTrackPlayer backing;
    PlaybackRenderer renderer{sequencer, engine, metronome, backing};
};

TEST_F(LongRunTest, Q06_DrumsAndBackingTrackStayOnTheSameSampleForTenMinutes)
{
    // Driver block sizes that do not divide a bar, changing from block to block.
    const std::vector<int> blockSizes{441, 128, 1000, 64, 2048, 333};
    const std::int64_t lastBarStart = (kBars - 1) * kDeviceSamplesPerBar;
    ASSERT_GE(lastBarStart, static_cast<std::int64_t>(10 * 60 * kDeviceRate));

    StereoOutput out(2048);
    renderer.render(&snapshot, out.channels.data(), 2, 1, 0.0); // the engine picks up the kit
    sequencer.play({.song = true});

    std::vector<std::int64_t> drumHits;
    std::vector<std::int64_t> backingClicks;
    std::int64_t songSample = 0;
    bool previousDrums = false;
    for (std::size_t block = 0; songSample < lastBarStart + 16; ++block)
    {
        const int frames = blockSizes[block % blockSizes.size()];
        std::fill(out.left.begin(), out.left.end(), 0.0F);
        std::fill(out.right.begin(), out.right.end(), 0.0F);
        renderer.render(&snapshot, out.channels.data(), 2, frames, 0.0);
        for (int index = 0; index < frames; ++index)
        {
            const float drums =
                out.right[static_cast<std::size_t>(index)]; // the backing plays on the left only
            const float click = out.left[static_cast<std::size_t>(index)] - drums;
            if (drums != 0.0F && !previousDrums)
                drumHits.push_back(songSample + index);
            previousDrums = drums != 0.0F;
            if (std::abs(click - 1.0F) < 1.0e-4F)
                backingClicks.push_back(songSample + index);
        }
        songSample += frames;
    }

    ASSERT_EQ(drumHits.size(), static_cast<std::size_t>(kBars));
    ASSERT_EQ(backingClicks.size(), static_cast<std::size_t>(kBars));
    for (int bar = 0; bar < kBars; ++bar)
    {
        const auto expected = bar * kDeviceSamplesPerBar;
        ASSERT_EQ(drumHits[static_cast<std::size_t>(bar)], expected) << "bar " << bar;
        ASSERT_EQ(backingClicks[static_cast<std::size_t>(bar)], expected) << "bar " << bar;
    }
}

} // namespace
} // namespace drumprog::engine
