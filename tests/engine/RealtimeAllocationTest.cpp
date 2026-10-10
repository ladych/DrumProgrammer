#include "engine/PlaybackRenderer.h"

#include "engine/FakeAudioFileStream.h"
#include "engine/SampleEngine.h"

#include "engine/RenderHelpers.h"
#include "input/EngineNoteSinks.h"
#include "input/MidiInputHandler.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>

namespace
{

// Counts heap allocations of the current thread while counting is switched on.
thread_local bool countAllocations = false;
thread_local int allocationCount = 0;

} // namespace

void* operator new(std::size_t size)
{
    if (countAllocations)
        ++allocationCount;
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t /*size*/) noexcept
{
    std::free(memory);
}

namespace drumprog::engine
{
namespace
{

std::unique_ptr<EngineKit> makeKit()
{
    auto sample =
        std::make_shared<const SampleBuffer>(SampleBuffer{48000.0, {std::vector<float>(5000, 0.1F)}});
    return std::make_unique<EngineKit>(
        std::vector<EngineSlot>{EngineSlot{.slotIndex = 0, .midiNote = 42, .sample = sample, .chokeGroup = 1},
                                EngineSlot{.slotIndex = 1, .midiNote = 46, .sample = sample, .chokeGroup = 1},
                                EngineSlot{.slotIndex = 2, .midiNote = 36, .sample = sample}});
}

TEST(RealtimeAllocationTest, Q04_RenderDoesNotAllocate)
{
    SampleEngine engine;
    engine.prepare(48000.0);
    engine.setKit(makeKit());
    StereoOutput out(128);

    countAllocations = true;
    for (int block = 0; block < 200; ++block)
    {
        for (int hit = 0; hit < 3; ++hit)
            engine.trigger(36, 100, hit * 10); // overflows the voice pool
        engine.trigger(block % 2 == 0 ? 46 : 42, 100, 5);
        engine.render(out.channels.data(), 2, 128);
    }
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
}

TEST(RealtimeAllocationTest, Q04_KitSwapAndQueuedTriggersDoNotAllocateOnAudioThread)
{
    SampleEngine engine;
    engine.prepare(48000.0);
    engine.setKit(makeKit());
    StereoOutput out(128);
    engine.render(out.channels.data(), 2, 128);

    for (int block = 0; block < 20; ++block)
    {
        engine.setKit(makeKit());
        engine.queueTrigger(36, 100);
        countAllocations = true;
        engine.render(out.channels.data(), 2, 128);
        countAllocations = false;
        engine.collectGarbage();
    }

    EXPECT_EQ(allocationCount, 0);
}

TEST(RealtimeAllocationTest, Q04_FIN03_MidiHitsDoNotAllocateOnMidiOrAudioThread)
{
    SampleEngine engine;
    engine.prepare(48000.0);
    engine.setKit(makeKit());
    input::MidiNoteSink sink{engine, &engine::steadyClockSeconds};
    input::InputActivity activity;
    input::MidiInputHandler handler{sink, activity};
    StereoOutput out(128);
    engine.render(out.channels.data(), 2, 128);
    const std::array<std::uint8_t, 3> noteOn{0x99, 36, 100};

    countAllocations = true;
    for (int block = 0; block < 20; ++block)
    {
        handler.handleMessage(noteOn);
        engine.render(out.channels.data(), 2, 128);
    }
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
}

TEST(RealtimeAllocationTest, Q04_FTR06_FIN07_SequencerPlaybackAndRecordingDoNotAllocate)
{
    ProjectSnapshot snapshot;
    snapshot.bpm = 300.0;
    snapshot.ticksPerQuarter = 960;
    snapshot.patterns.push_back({3840, {NoteSnapshot{2, 0, 240, 100}, NoteSnapshot{0, 480, 240, 100}}});
    Sequencer sequencer;
    SampleEngine engine;
    Metronome metronome;
    BackingTrackPlayer backing;
    PlaybackRenderer renderer{sequencer, engine, metronome, backing};
    renderer.prepare(48000.0);
    engine.setKit(makeKit());
    sequencer.setMetronome(true, true);
    sequencer.play({.take = 1, .countInBars = 1});
    StereoOutput out(128);

    countAllocations = true;
    for (int block = 0; block < 2000; ++block)
    {
        engine.queueLiveTrigger(36, 100, 0.001 * block);
        renderer.render(&snapshot, out.channels.data(), 2, 128, 0.001 * block);
    }
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
    RecordedHit hit;
    EXPECT_TRUE(sequencer.popRecordedHit(hit));
}

TEST(RealtimeAllocationTest, Q04_FBT02_FBT07_BackingTrackAndSongRecordingDoNotAllocate)
{
    ProjectSnapshot snapshot;
    snapshot.bpm = 120.0;
    snapshot.ticksPerQuarter = 960;
    snapshot.patterns.push_back({3840, {NoteSnapshot{0, 0, 240, 100}}});
    snapshot.song.push_back({0, 0, 3840});
    snapshot.songLengthTicks = 3840;
    Sequencer sequencer;
    SampleEngine engine;
    Metronome metronome;
    BackingTrackPlayer backing;
    PlaybackRenderer renderer{sequencer, engine, metronome, backing};
    renderer.prepare(48000.0);
    engine.setKit(makeKit());
    backing.setStream(FakeAudioFileStream::ramp(44100.0, 100000));
    sequencer.play({.take = 1, .countInBars = 1, .song = true});
    StereoOutput out(1024);

    countAllocations = true;
    for (int block = 0; block < 400; ++block)
    {
        engine.queueLiveTrigger(36, 100, 0.02 * block);
        renderer.render(&snapshot, out.channels.data(), 2, 1024, 0.02 * block);
    }
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
    RecordedHit hit;
    EXPECT_TRUE(sequencer.popRecordedHit(hit));
}

} // namespace
} // namespace drumprog::engine
