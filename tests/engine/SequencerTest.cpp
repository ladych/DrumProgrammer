#include "engine/Sequencer.h"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace drumprog::engine
{
namespace
{

constexpr double kRate = 48000.0;
// 120 BPM at 960 PPQ and 48 kHz: one tick = 25 samples, one beat = 24000, one 4/4 bar = 96000.
constexpr std::int64_t kTick = 25;
constexpr std::int64_t kBeat = 24000;
constexpr std::int64_t kBar = 96000;
constexpr std::int64_t kBarTicks = 3840;

struct Event
{
    std::int64_t sample = 0;
    int slotOrAccent = 0;

    bool operator==(const Event&) const = default;
};

ProjectSnapshot makeSnapshot(std::vector<NoteSnapshot> notes, std::int64_t lengthTicks = kBarTicks)
{
    ProjectSnapshot snapshot;
    snapshot.bpm = 120.0;
    snapshot.ticksPerQuarter = 960;
    snapshot.patterns.push_back({lengthTicks, std::move(notes)});
    return snapshot;
}

NoteSnapshot note(std::int64_t tick, int slot = 0, int velocity = 100)
{
    return {slot, tick, 240, velocity};
}

class SequencerTest : public ::testing::Test
{
protected:
    void SetUp() override { sequencer.prepare(kRate); }

    /// Runs numSamples in blocks and returns notes and clicks with their absolute sample.
    void run(std::int64_t numSamples, int blockSize = 128)
    {
        for (std::int64_t done = 0; done < numSamples; done += blockSize)
        {
            const int size = static_cast<int>(std::min<std::int64_t>(blockSize, numSamples - done));
            sequencer.process(&snapshot, size, block);
            for (std::size_t index = 0; index < block.numNotes; ++index)
                notes.push_back({now + block.notes.at(index).sampleOffset, block.notes.at(index).slotIndex});
            for (std::size_t index = 0; index < block.numClicks; ++index)
                clicks.push_back(
                    {now + block.clicks.at(index).sampleOffset, block.clicks.at(index).accent ? 1 : 0});
            now += size;
        }
    }

    void processBlock(int size = 128)
    {
        sequencer.process(&snapshot, size, block);
        now += size;
    }

    std::vector<RecordedHit> recordedHits()
    {
        std::vector<RecordedHit> hits;
        RecordedHit hit;
        while (sequencer.popRecordedHit(hit))
            hits.push_back(hit);
        return hits;
    }

    Sequencer sequencer;
    ProjectSnapshot snapshot = makeSnapshot({note(0, 0), note(1, 1), note(10, 2), note(1920, 3)});
    SequencerBlock block;
    std::int64_t now = 0;
    std::vector<Event> notes;
    std::vector<Event> clicks;
};

TEST_F(SequencerTest, FTR01_StoppedTransportPlaysNothing)
{
    run(kBar);
    EXPECT_TRUE(notes.empty());
    EXPECT_EQ(sequencer.state(), TransportState::stopped);
    EXPECT_EQ(sequencer.position(), 0);
}

TEST_F(SequencerTest, Q04_WaitsForTheFirstSnapshot)
{
    sequencer.play({});
    sequencer.process(nullptr, 128, block);
    EXPECT_EQ(block.numNotes, 0U);
    EXPECT_EQ(sequencer.state(), TransportState::stopped);

    processBlock();
    EXPECT_EQ(sequencer.state(), TransportState::playing);
}

TEST_F(SequencerTest, FTR06_NotesLandOnTheirExactSample)
{
    sequencer.play({});
    run(kBar);
    EXPECT_EQ(notes, (std::vector<Event>{{0, 0}, {kTick, 1}, {10 * kTick, 2}, {2 * kBeat, 3}}));
}

TEST_F(SequencerTest, FTR06_Q06_BlockSizeDoesNotMoveAnyEvent)
{
    snapshot.bpm = 97.31; // a tick is a fraction of a sample
    sequencer.setMetronome(true, true);
    sequencer.play({});
    run(5 * kBar, 128);
    const auto expectedNotes = notes;
    const auto expectedClicks = clicks;
    ASSERT_GE(expectedNotes.size(), 4U * 4U);

    for (const int blockSize : {1, 64, 100, 997, 4096})
    {
        Sequencer other;
        other.prepare(kRate);
        other.setMetronome(true, true);
        other.play({});
        std::vector<Event> otherNotes;
        std::int64_t position = 0;
        while (position < 5 * kBar)
        {
            other.process(&snapshot, blockSize, block);
            for (std::size_t index = 0; index < block.numNotes; ++index)
                otherNotes.push_back(
                    {position + block.notes.at(index).sampleOffset, block.notes.at(index).slotIndex});
            position += blockSize;
        }
        otherNotes.resize(std::min(otherNotes.size(), expectedNotes.size()));
        EXPECT_EQ(otherNotes, expectedNotes) << "block size " << blockSize;
    }
    EXPECT_FALSE(expectedClicks.empty());
}

TEST_F(SequencerTest, FTR02_LoopRepeatsThePattern)
{
    snapshot = makeSnapshot({note(0)});
    sequencer.play({});
    run(2 * kBar + kBeat);
    EXPECT_EQ(notes, (std::vector<Event>{{0, 0}, {kBar, 0}, {2 * kBar, 0}}));
    EXPECT_TRUE(sequencer.loop());
}

TEST_F(SequencerTest, FTR02_WithoutLoopTheTransportStopsAtThePatternEnd)
{
    snapshot = makeSnapshot({note(0)});
    sequencer.setLoop(false);
    sequencer.play({});
    run(kBar - 128);
    EXPECT_EQ(sequencer.state(), TransportState::playing);
    run(2 * kBar);
    EXPECT_EQ(notes, (std::vector<Event>{{0, 0}}));
    EXPECT_EQ(sequencer.state(), TransportState::stopped);
    EXPECT_EQ(sequencer.position(), 0);
}

TEST_F(SequencerTest, FTR02_SwitchingLoopOffEndsTheCurrentPass)
{
    snapshot = makeSnapshot({note(0)});
    sequencer.play({});
    run(kBar + kBeat);
    sequencer.setLoop(false);
    run(2 * kBar);
    EXPECT_EQ(notes, (std::vector<Event>{{0, 0}, {kBar, 0}}));
    EXPECT_EQ(sequencer.state(), TransportState::stopped);
}

TEST_F(SequencerTest, FTR06_NotesBeyondThePatternLengthAreNotPlayed)
{
    snapshot = makeSnapshot({note(100), note(kBarTicks), note(kBarTicks + 5)});
    sequencer.play({});
    run(2 * kBar);
    EXPECT_EQ(notes, (std::vector<Event>{{100 * kTick, 0}, {kBar + 100 * kTick, 0}}));
}

TEST_F(SequencerTest, FTR06_PatternOutsideTheSnapshotPlaysNothingForOneBar)
{
    sequencer.setLoop(false);
    sequencer.play({.patternIndex = 3});
    run(kBar + 128);
    EXPECT_TRUE(notes.empty());
    EXPECT_EQ(sequencer.state(), TransportState::stopped);

    sequencer.play({.patternIndex = -1});
    processBlock();
    EXPECT_EQ(sequencer.state(), TransportState::playing);
}

TEST_F(SequencerTest, FTR01_StopKeepsThePositionAndPlayContinuesThere)
{
    sequencer.play({});
    run(100 * kTick);
    sequencer.stop();
    processBlock();
    EXPECT_EQ(sequencer.state(), TransportState::stopped);
    EXPECT_EQ(sequencer.position(), 100);

    notes.clear();
    now = 0;
    sequencer.play({});
    run(kBeat);
    // The note at tick 1920 is next, 1820 ticks after the paused position.
    EXPECT_EQ(notes, (std::vector<Event>{}));
    run(kBeat);
    EXPECT_EQ(notes, (std::vector<Event>{{1820 * kTick, 3}}));
}

TEST_F(SequencerTest, FTR01_PlayWhilePlayingAndStopWhileStoppedAreIgnored)
{
    sequencer.stop();
    processBlock();
    sequencer.play({});
    run(100 * kTick);
    sequencer.play({.take = 5});
    processBlock();
    EXPECT_EQ(sequencer.activeTake(), 0U);
    EXPECT_EQ(sequencer.position(), 105);
}

TEST_F(SequencerTest, FTR01_RewindWhileStoppedGoesBackToTheStart)
{
    sequencer.play({});
    run(100 * kTick);
    sequencer.stop();
    sequencer.rewind();
    processBlock();
    EXPECT_EQ(sequencer.position(), 0);
}

TEST_F(SequencerTest, FTR01_RewindWhilePlayingJumpsToTheStart)
{
    sequencer.play({});
    run(1000 * kTick);
    notes.clear();
    sequencer.rewind();
    const std::int64_t rewoundAt = now;
    run(kBeat);
    EXPECT_EQ(notes.front(), (Event{rewoundAt, 0}));
}

TEST_F(SequencerTest, FTR01_CommandQueueRejectsCommandsWhenFull)
{
    for (std::size_t index = 0; index < Sequencer::kCommandQueueSize; ++index)
        ASSERT_TRUE(sequencer.stop());
    EXPECT_FALSE(sequencer.play({}));
    EXPECT_FALSE(sequencer.rewind());
}

TEST_F(SequencerTest, FTR04_PositionFollowsThePlaybackAndWraps)
{
    sequencer.play({});
    run(960 * kTick);
    EXPECT_EQ(sequencer.position(), 960);
    run(kBar);
    EXPECT_EQ(sequencer.position(), 960);
}

TEST_F(SequencerTest, FTR03_TempoChangeKeepsThePosition)
{
    sequencer.play({});
    run(40 * kTick);
    snapshot.bpm = 60.0; // one tick = 50 samples
    snapshot.patterns[0].notes = {note(100)};
    const std::int64_t changedAt = now;
    run(kBeat);
    EXPECT_EQ(notes.back(), (Event{changedAt + 60 * 50, 0}));
}

TEST_F(SequencerTest, FTR06_SampleRateChangeKeepsThePosition)
{
    snapshot.patterns[0].notes = {note(100)};
    sequencer.play({});
    run(40 * kTick);
    sequencer.prepare(96000.0);
    const std::int64_t changedAt = now;
    run(kBeat);
    EXPECT_EQ(notes, (std::vector<Event>{{changedAt + 60 * 50, 0}}));
}

TEST_F(SequencerTest, FTR08_MetronomeClicksEveryBeatWithAccentOnTheBar)
{
    sequencer.setMetronome(true, false);
    sequencer.play({});
    run(kBar + 128);
    EXPECT_EQ(clicks, (std::vector<Event>{{0, 1}, {kBeat, 0}, {2 * kBeat, 0}, {3 * kBeat, 0}, {kBar, 1}}));
}

TEST_F(SequencerTest, FTR08_MetronomeFollowsTheTimeSignature)
{
    snapshot.timeSigNumerator = 6;
    snapshot.timeSigDenominator = 8;
    sequencer.setMetronome(true, false);
    sequencer.play({});
    run(6 * kBeat / 2 + 128);
    ASSERT_EQ(clicks.size(), 7U);
    EXPECT_EQ(clicks[1], (Event{kBeat / 2, 0}));
    EXPECT_EQ(clicks[6], (Event{3 * kBeat, 1}));
}

TEST_F(SequencerTest, FTR08_MetronomeIsSwitchedSeparatelyForPlaybackAndRecording)
{
    sequencer.setMetronome(false, true);
    sequencer.play({});
    run(kBar);
    EXPECT_TRUE(clicks.empty());
    sequencer.stop();
    sequencer.rewind();
    sequencer.play({.take = 1});
    run(kBeat + 1);
    EXPECT_EQ(clicks.size(), 2U);

    sequencer.stop();
    sequencer.setMetronome(true, false);
    clicks.clear();
    sequencer.play({.take = 2});
    run(kBar);
    EXPECT_TRUE(clicks.empty());
}

TEST_F(SequencerTest, FTR09_CountInClicksBeforeTheRecordingStarts)
{
    sequencer.setMetronome(false, false);
    sequencer.play({.take = 1, .countInBars = 1});
    run(kBeat);
    EXPECT_EQ(sequencer.state(), TransportState::countIn);
    EXPECT_EQ(sequencer.position(), 0);
    EXPECT_EQ(sequencer.activeTake(), 1U);
    run(kBar);
    EXPECT_EQ(sequencer.state(), TransportState::playing);
    EXPECT_EQ(clicks, (std::vector<Event>{{0, 1}, {kBeat, 0}, {2 * kBeat, 0}, {3 * kBeat, 0}}));
    EXPECT_EQ(notes.front(), (Event{kBar, 0}));
}

TEST_F(SequencerTest, FTR09_CountInIsLimitedToTwoBarsAndOnlyBeforeRecording)
{
    sequencer.play({.take = 1, .countInBars = 5});
    run(2 * kBar - 128);
    EXPECT_EQ(sequencer.state(), TransportState::countIn);
    run(128);
    EXPECT_EQ(sequencer.state(), TransportState::playing);

    sequencer.stop();
    sequencer.rewind();
    notes.clear();
    sequencer.play({.countInBars = 2});
    run(128);
    EXPECT_EQ(sequencer.state(), TransportState::playing);
    EXPECT_FALSE(notes.empty());
}

TEST_F(SequencerTest, FTR09_RewindDuringCountInKeepsTheCountInLeft)
{
    snapshot = makeSnapshot({note(0)});
    sequencer.play({});
    run(100 * kTick);
    sequencer.stop();
    sequencer.play({.take = 1, .countInBars = 1});
    run(kBeat);
    sequencer.rewind();
    notes.clear();
    const std::int64_t rewoundAt = now;
    run(kBar);
    EXPECT_EQ(notes, (std::vector<Event>{{rewoundAt + 3 * kBeat, 0}}));
}

TEST_F(SequencerTest, FTR09_StopDuringCountInKeepsTheStartPosition)
{
    sequencer.play({});
    run(100 * kTick);
    sequencer.stop();
    sequencer.play({.take = 1, .countInBars = 1});
    run(kBeat);
    sequencer.stop();
    processBlock();
    EXPECT_EQ(sequencer.position(), 100);
    EXPECT_EQ(sequencer.activeTake(), 0U);
}

TEST_F(SequencerTest, Q04_EventsBeyondTheBlockCapacityAreDropped)
{
    snapshot = makeSnapshot(std::vector<NoteSnapshot>(SequencerBlock::kMaxNotes + 10, note(0)));
    sequencer.play({});
    processBlock();
    EXPECT_EQ(block.numNotes, SequencerBlock::kMaxNotes);

    sequencer.setMetronome(true, true);
    sequencer.process(&snapshot, static_cast<int>(3 * kBar), block);
    EXPECT_EQ(block.numClicks, SequencerBlock::kMaxClicks);
}

TEST_F(SequencerTest, Q04_GuiAndAudioThreadShareTheSequencer)
{
    std::atomic<bool> running{true};
    std::thread audio(
        [this, &running]
        {
            SequencerBlock audioBlock;
            const std::array<LiveHit, 1> hits{LiveHit{0, 100, 0.0}};
            while (running.load())
            {
                sequencer.process(&snapshot, 64, audioBlock);
                sequencer.record(hits, 0.0);
            }
        });
    RecordedHit hit;
    for (std::uint32_t take = 1; take < 200; ++take)
    {
        sequencer.play({.take = take, .countInBars = 1});
        sequencer.setLoop(take % 2 == 0);
        sequencer.setMetronome(true, take % 3 == 0);
        sequencer.setLatencyCompensation(0.001 * take);
        std::this_thread::yield();
        while (sequencer.popRecordedHit(hit))
            EXPECT_GE(hit.tick, 0);
        EXPECT_GE(sequencer.position(), 0);
        sequencer.stop();
        sequencer.rewind();
    }
    running.store(false);
    audio.join();
    SUCCEED();
}

class SequencerRecordTest : public SequencerTest
{
protected:
    /// Processes one block of 480 samples (19.2 ticks; 1920 ticks per second) and records hits played
    /// secondsAgo before it.
    void recordBlock(std::initializer_list<double> secondsAgo, int slot = 2)
    {
        processBlock(480);
        std::vector<LiveHit> hits;
        for (const double ago : secondsAgo)
            hits.push_back({slot, 90, kBlockTime - ago});
        sequencer.record(hits, kBlockTime);
    }

    static constexpr double kBlockTime = 100.0;
};

TEST_F(SequencerRecordTest, FIN07_NothingIsRecordedWithoutTake)
{
    recordBlock({0.0});
    sequencer.play({});
    recordBlock({0.0});
    EXPECT_TRUE(recordedHits().empty());
}

TEST_F(SequencerRecordTest, FIN07_FIN08_HitIsPlacedWhereItWasPlayed)
{
    sequencer.play({.take = 7});
    recordBlock({});
    // Block starts at tick 19.2; a hit 10 ms earlier was played at tick 0.
    recordBlock({0.01, 0.0});
    const auto hits = recordedHits();
    ASSERT_EQ(hits.size(), 2U);
    EXPECT_EQ(hits[0].take, 7U);
    EXPECT_EQ(hits[0].slotIndex, 2);
    EXPECT_EQ(hits[0].velocity, 90);
    EXPECT_EQ(hits[0].tick, 0);
    EXPECT_EQ(hits[1].tick, 19);
}

TEST_F(SequencerRecordTest, FIN08_LatencyCompensationMovesHitsBack)
{
    sequencer.setLatencyCompensation(0.005); // 9.6 ticks
    sequencer.play({.take = 1});
    recordBlock({});
    recordBlock({0.0});
    const auto hits = recordedHits();
    ASSERT_EQ(hits.size(), 1U);
    EXPECT_EQ(hits[0].tick, 10);
}

TEST_F(SequencerRecordTest, FIN10_HitsJustBeforeTheStartFoldToThePatternEnd)
{
    sequencer.play({.take = 1, .countInBars = 1});
    run(kBar - 480);
    recordBlock({0.4, 0.0}); // 768 ticks early: count-in; 19.2 ticks early: folded
    const auto hits = recordedHits();
    ASSERT_EQ(hits.size(), 1U);
    EXPECT_EQ(hits[0].tick, kBarTicks - 19);
}

TEST_F(SequencerRecordTest, FIN10_LateHitsAfterTheLoopEndFoldToTheStart)
{
    sequencer.play({.take = 1});
    run(kBar);
    recordBlock({0.0});
    const auto hits = recordedHits();
    ASSERT_EQ(hits.size(), 1U);
    EXPECT_EQ(hits[0].tick, 0);
}

TEST_F(SequencerRecordTest, FIN07_HitsOfTheLastBlockAreRecordedWhenThePatternEnds)
{
    sequencer.setLoop(false);
    sequencer.play({.take = 3});
    run(kBar - 240);
    recordBlock({0.0});
    EXPECT_EQ(sequencer.state(), TransportState::stopped);
    EXPECT_EQ(sequencer.activeTake(), 0U);
    const auto hits = recordedHits();
    ASSERT_EQ(hits.size(), 1U);
    EXPECT_EQ(hits[0].take, 3U);
}

// ----- Song mode (F-TR-05) -------------------------------------------------------------------

class SequencerSongTest : public SequencerTest
{
protected:
    void SetUp() override
    {
        SequencerTest::SetUp();
        // Pattern 0 "verse" plays slot 0 on every beat, pattern 1 "fill" slot 1 on the first beat.
        snapshot = makeSnapshot({note(0, 0), note(960, 0), note(1920, 0), note(2880, 0)});
        snapshot.patterns.push_back({kBarTicks, {note(0, 1)}});
        setSong({{1, 0, kBarTicks}, {0, 2 * kBarTicks, kBarTicks}});
    }

    void setSong(std::vector<SongEntrySnapshot> entries)
    {
        snapshot.songLengthTicks = 0;
        for (const auto& entry : entries)
            snapshot.songLengthTicks =
                std::max(snapshot.songLengthTicks, entry.startTick + entry.lengthTicks);
        snapshot.song = std::move(entries);
    }
};

TEST_F(SequencerSongTest, FTR05_PlaysTheBlocksOfTheSongAndLoopsIt)
{
    sequencer.play({.song = true});
    run(3 * kBar + kBeat / 2);
    EXPECT_EQ(notes,
              (std::vector<Event>{{0, 1},
                                  {2 * kBar, 0},
                                  {2 * kBar + kBeat, 0},
                                  {2 * kBar + 2 * kBeat, 0},
                                  {2 * kBar + 3 * kBeat, 0},
                                  {3 * kBar, 1}}));
}

TEST_F(SequencerSongTest, FTR05_WithoutLoopTheSongStopsAtItsEnd)
{
    sequencer.setLoop(false);
    sequencer.play({.song = true});
    run(3 * kBar - 128);
    EXPECT_EQ(sequencer.state(), TransportState::playing);
    run(kBar);
    EXPECT_EQ(notes.size(), 5U);
    EXPECT_EQ(sequencer.state(), TransportState::stopped);
    EXPECT_EQ(sequencer.position(), 0);
}

TEST_F(SequencerSongTest, FTR04_PositionCountsFromTheSongStart)
{
    sequencer.play({.song = true});
    run(2 * kBar + kBeat);
    EXPECT_EQ(sequencer.position(), 2 * kBarTicks + 960);
    sequencer.stop();
    processBlock();
    EXPECT_EQ(sequencer.position(), 2 * kBarTicks + 960);
    sequencer.play({.song = true});
    run(kBeat);
    EXPECT_EQ(sequencer.position(), 2 * kBarTicks + 2 * 960);
}

TEST_F(SequencerSongTest, FTR05_ABlockOnlyPlaysItsPlayedLength)
{
    setSong({{0, 0, kBarTicks / 2}, {1, kBarTicks / 2, kBarTicks}});
    sequencer.play({.song = true});
    run(kBar + kBar / 2 - 128);
    EXPECT_EQ(notes, (std::vector<Event>{{0, 0}, {kBeat, 0}, {2 * kBeat, 1}}));
}

TEST_F(SequencerSongTest, FTR05_BlocksOfPatternsOutsideTheSnapshotAreSilent)
{
    setSong({{5, 0, kBarTicks}, {-1, kBarTicks, kBarTicks}, {1, 2 * kBarTicks, kBarTicks}});
    sequencer.setLoop(false);
    sequencer.play({.song = true});
    run(4 * kBar);
    EXPECT_EQ(notes, (std::vector<Event>{{2 * kBar, 1}}));
}

TEST_F(SequencerSongTest, FTR05_AnEmptySongPlaysOneSilentBar)
{
    setSong({});
    sequencer.setLoop(false);
    sequencer.setMetronome(true, true);
    sequencer.play({.song = true});
    run(2 * kBar);
    EXPECT_TRUE(notes.empty());
    EXPECT_EQ(clicks.size(), 4U);
    EXPECT_EQ(sequencer.state(), TransportState::stopped);
}

TEST_F(SequencerSongTest, FTR05_TheSongModeNeverRecords)
{
    sequencer.play({.take = 4, .countInBars = 1, .song = true});
    processBlock();
    EXPECT_EQ(sequencer.state(), TransportState::playing);
    EXPECT_EQ(sequencer.activeTake(), 0U);
    sequencer.record(std::array{LiveHit{0, 100, 0.0}}, 0.0);
    EXPECT_TRUE(recordedHits().empty());
}

TEST_F(SequencerSongTest, FTR05_ThePatternModeIgnoresTheSong)
{
    sequencer.play({.patternIndex = 1});
    run(2 * kBar);
    EXPECT_EQ(notes, (std::vector<Event>{{0, 1}, {kBar, 1}}));
}

} // namespace
} // namespace drumprog::engine
