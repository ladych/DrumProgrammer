#include "engine/PlaybackRenderer.h"

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
    PlaybackRenderer renderer{sequencer, engine, metronome};
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

} // namespace
} // namespace drumprog::engine
