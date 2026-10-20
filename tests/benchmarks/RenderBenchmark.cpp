#include "engine/PlaybackRenderer.h"

#include "engine/FakeAudioFileStream.h"
#include "engine/RenderHelpers.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace drumprog::engine
{
namespace
{

constexpr double kDeviceRate = 48000.0;
constexpr int kBlockSize = 128;
constexpr int kVoices = 16;
constexpr double kSeconds = 60.0;
/// Q-05: at most 15 % of one core.
constexpr double kMaxLoad = 0.15;

std::vector<float> noise(int frames, unsigned seed)
{
    std::vector<float> data(static_cast<std::size_t>(frames));
    for (auto& value : data)
    {
        seed = seed * 1664525U + 1013904223U;
        value = static_cast<float>(seed >> 8U) / static_cast<float>(1U << 24U) - 0.5F;
    }
    return data;
}

/// Q-05: the audio callback with 16 voices playing at the same time and a backing track. Each of the 16 slots
/// has a stereo sample of 2 seconds, pitched down so the voices interpolate and last longer than a bar; a
/// pattern of one bar (2 seconds at 120 BPM) starts all of them on its first tick, and each slot chokes its
/// own previous voice, so 16 voices play all the time. The backing track is a
/// stereo file at 44.1 kHz and is resampled to the device rate. Measured is the time render() takes for
/// one minute of audio in blocks of 128 samples, as a share of that minute.
TEST(RenderBenchmark, Q05_SixteenVoicesAndBackingTrackNeedAtMostFifteenPercentOfOneCore)
{
#ifndef NDEBUG
    GTEST_SKIP() << "only meaningful in an optimised build";
#endif
    constexpr int kSampleFrames = 2 * 44100;
    std::vector<EngineSlot> slots;
    PatternSnapshot pattern{3840, {}};
    for (int slot = 0; slot < kVoices; ++slot)
    {
        auto sample = std::make_shared<const SampleBuffer>(
            SampleBuffer{44100.0,
                         {noise(kSampleFrames, static_cast<unsigned>(2 * slot + 1)),
                          noise(kSampleFrames, static_cast<unsigned>(2 * slot + 2))}});
        slots.push_back(EngineSlot{.slotIndex = slot,
                                   .midiNote = 36 + slot,
                                   .sample = sample,
                                   .playbackRate = 0.94,
                                   .chokeGroup = slot + 1});
        pattern.notes.push_back(NoteSnapshot{slot, 0, 240, 100});
    }

    ProjectSnapshot snapshot;
    snapshot.bpm = 120.0;
    snapshot.ticksPerQuarter = 960;
    snapshot.patterns.push_back(pattern);
    snapshot.song.push_back({0, 0, 3840});
    snapshot.songLengthTicks = 3840;

    Sequencer sequencer;
    SampleEngine engine;
    Metronome metronome;
    BackingTrackPlayer backing;
    PlaybackRenderer renderer{sequencer, engine, metronome, backing};
    engine.setKit(std::make_unique<EngineKit>(std::move(slots)));
    const int backingFrames = static_cast<int>(kSeconds * 44100.0) + 44100;
    backing.setStream(
        std::make_unique<FakeAudioFileStream>(44100.0, noise(backingFrames, 99U), noise(backingFrames, 98U)));
    renderer.prepare(kDeviceRate);
    sequencer.setLoop(true);
    sequencer.setMetronome(true, true);

    StereoOutput out(kBlockSize);
    renderer.render(&snapshot, out.channels.data(), 2, kBlockSize, 0.0); // the engine picks up the kit
    sequencer.play({.song = true});

    const int blocks = static_cast<int>(kSeconds * kDeviceRate) / kBlockSize;
    const auto start = std::chrono::steady_clock::now();
    for (int block = 0; block < blocks; ++block)
    {
        std::fill(out.left.begin(), out.left.end(), 0.0F);
        std::fill(out.right.begin(), out.right.end(), 0.0F);
        renderer.render(&snapshot, out.channels.data(), 2, kBlockSize, 0.0);
    }
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

    const double load = elapsed.count() / kSeconds;
    std::printf("Q-05: %d voices + backing track: %.2f %% of one core (%.3f s for %.0f s of audio)\n",
                kVoices,
                100.0 * load,
                elapsed.count(),
                kSeconds);
    ::testing::Test::RecordProperty("load_percent", std::to_string(100.0 * load));
    EXPECT_LE(load, kMaxLoad);
}

} // namespace
} // namespace drumprog::engine
