#include "engine/PlaybackRenderer.h"

#include "engine/FakeAudioFileStream.h"
#include "engine/RenderHelpers.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace drumprog::engine
{
namespace
{

constexpr double kRate = 48000.0;

class PlaybackRendererTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto sample =
            std::make_shared<const SampleBuffer>(SampleBuffer{kRate, {std::vector<float>(100, 1.0F)}});
        engine.setKit(std::make_unique<EngineKit>(
            std::vector<EngineSlot>{EngineSlot{.slotIndex = 0, .midiNote = 36, .sample = sample}}));
        snapshot.bpm = 120.0;
        snapshot.ticksPerQuarter = 960;
        snapshot.patterns.push_back({3840, {NoteSnapshot{0, 2, 240, 127}}}); // tick 2 = sample 50
        renderer.prepare(kRate);
        render(1); // the engine picks up the kit
    }

    StereoOutput render(int frames, double time = 0.0)
    {
        StereoOutput out(frames);
        renderer.render(&snapshot, out.channels.data(), 2, frames, time);
        return out;
    }

    ProjectSnapshot snapshot;
    Sequencer sequencer;
    SampleEngine engine;
    Metronome metronome;
    BackingTrackPlayer backing;
    PlaybackRenderer renderer{sequencer, engine, metronome, backing};
};

TEST_F(PlaybackRendererTest, FTR06_PlaysTheSnapshotNotesOnTheirSample)
{
    sequencer.play({});
    const auto out = render(64);
    EXPECT_EQ(out.left[49], 0.0F);
    EXPECT_FLOAT_EQ(out.left[50], 1.0F);
    EXPECT_FLOAT_EQ(out.right[63], 1.0F);
}

TEST_F(PlaybackRendererTest, FTR08_AddsTheMetronomeClicks)
{
    sequencer.setMetronome(true, true);
    metronome.setLevel(1.0F);
    sequencer.play({});
    const auto out = render(40);
    EXPECT_NE(out.left[1], 0.0F);
}

TEST_F(PlaybackRendererTest, FIN07_RecordsTheLiveHitsOfTheBlock)
{
    sequencer.play({.take = 4});
    render(480, 10.0);
    engine.queueLiveTrigger(36, 80, 10.0);
    render(480, 10.01); // block start tick 19.2, hit 10 ms = 19.2 ticks before
    RecordedHit hit;
    ASSERT_TRUE(sequencer.popRecordedHit(hit));
    EXPECT_EQ(hit.take, 4U);
    EXPECT_EQ(hit.velocity, 80);
    EXPECT_EQ(hit.tick, 0);
}

TEST_F(PlaybackRendererTest, FBT06_DrumsGetTheDrumsAndTheMasterGain)
{
    snapshot.mix = {.backingGain = 0.1, .drumsGain = 0.5, .masterGain = 0.5};
    sequencer.play({});
    const auto out = render(64);
    EXPECT_FLOAT_EQ(out.left[50], 0.25F);
}

TEST_F(PlaybackRendererTest, FBT06_TheMetronomeKeepsItsLevel)
{
    snapshot.mix = {.backingGain = 1.0, .drumsGain = 0.0, .masterGain = 0.0};
    sequencer.setMetronome(true, true);
    metronome.setLevel(1.0F);
    sequencer.play({});
    const auto out = render(40);
    EXPECT_NE(out.left[1], 0.0F);
    EXPECT_EQ(out.left[39], out.right[39]);
}

TEST_F(PlaybackRendererTest, FTR06_LongBlocksAreRenderedInSteps)
{
    snapshot.patterns[0].notes.push_back({0, 24, 240, 127}); // sample 600, in the second step
    sequencer.play({});
    const auto out = render(1200);
    EXPECT_EQ(out.left[149], 1.0F);
    EXPECT_EQ(out.left[150], 0.0F);
    EXPECT_FLOAT_EQ(out.left[600], 1.0F);
    EXPECT_FLOAT_EQ(out.left[699], 1.0F);
    EXPECT_EQ(out.left[700], 0.0F);
}

class PlaybackRendererBackingTest : public PlaybackRendererTest
{
protected:
    void SetUp() override
    {
        PlaybackRendererTest::SetUp();
        snapshot.patterns[0].notes.clear();
        snapshot.backingTrack = {.offsetSamples = 10, .gain = 0.5};
        snapshot.mix = {.backingGain = 0.5, .drumsGain = 1.0, .masterGain = 2.0};
        backing.setStream(FakeAudioFileStream::ramp(kRate, 200000));
    }
};

TEST_F(PlaybackRendererBackingTest, FBT03_FBT06_TheBackingTrackPlaysInTheSongModeWithItsGain)
{
    sequencer.play({.song = true});
    const auto out = render(1000);
    // Song sample n plays file frame n + 10 (value n + 11) at 0.5 * 0.5 * 2.
    EXPECT_FLOAT_EQ(out.left[0], 0.5F * 11.0F);
    EXPECT_FLOAT_EQ(out.left[999], 0.5F * 1010.0F);
    EXPECT_FLOAT_EQ(out.right[999], -0.5F * 1010.0F);
}

TEST_F(PlaybackRendererBackingTest, FBT03_TheTrackLengthExtendsTheSong)
{
    // An empty song lasts one bar; the track of 200000 frames needs three.
    sequencer.setLoop(false);
    sequencer.play({.song = true});
    render(2 * 96000 + 512);
    EXPECT_EQ(sequencer.state(), TransportState::playing);
}

TEST_F(PlaybackRendererBackingTest, FBT03_ThePatternModeAndTheStoppedTransportAreSilent)
{
    EXPECT_EQ(render(64).left[10], 0.0F);
    sequencer.play({});
    EXPECT_EQ(render(64).left[10], 0.0F);
}

TEST_F(PlaybackRendererBackingTest, FBT03_MonoOutputAndNoOutputs)
{
    sequencer.play({.song = true});
    std::vector<float> mono(4, 0.0F);
    std::array<float*, 1> channels{mono.data()};
    renderer.render(&snapshot, channels.data(), 1, 4, 0.0);
    EXPECT_FLOAT_EQ(mono[0], 0.0F); // left and right of the ramp cancel out
    renderer.render(&snapshot, channels.data(), 0, 4, 0.0);
    renderer.render(nullptr, channels.data(), 1, 4, 0.0);
    EXPECT_FLOAT_EQ(mono[3], 0.0F);
}

} // namespace
} // namespace drumprog::engine
