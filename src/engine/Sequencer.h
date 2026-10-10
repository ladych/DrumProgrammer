#pragma once

#include "engine/LiveHit.h"
#include "engine/ProjectSnapshot.h"
#include "engine/SpscQueue.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace drumprog::engine
{

enum class TransportState : std::uint8_t
{
    stopped,
    countIn, ///< clicks before the recording starts (F-TR-09)
    playing
};

struct SequencedNote
{
    int slotIndex = 0;
    int velocity = 0;
    int sampleOffset = 0;
};

struct MetronomeClick
{
    bool accent = false; ///< first beat of a bar
    int sampleOffset = 0;
};

/// Samples of a block that play the song continuously, for the backing track (F-BT-03). A block has a
/// second span where the song loops back to its start.
struct SongSpan
{
    int sampleOffset = 0;
    int numSamples = 0;
    std::int64_t songSample = 0; ///< song position of the first sample in device samples
};

/// What the sequencer plays in one audio block. Fixed capacity, so the audio thread never allocates;
/// events beyond it are dropped.
struct SequencerBlock
{
    static constexpr std::size_t kMaxNotes = 512;
    static constexpr std::size_t kMaxClicks = 8;
    static constexpr std::size_t kMaxSongSpans = 4;

    std::array<SequencedNote, kMaxNotes> notes{};
    std::size_t numNotes = 0;
    std::array<MetronomeClick, kMaxClicks> clicks{};
    std::size_t numClicks = 0;
    std::array<SongSpan, kMaxSongSpans> songSpans{}; ///< only in the song mode
    std::size_t numSongSpans = 0;
};

/// A recorded hit for the GUI thread, already placed in the pattern (F-IN-07, F-IN-08, F-IN-10) or in the
/// song (F-BT-07).
struct RecordedHit
{
    std::uint32_t take = 0;
    int slotIndex = 0;
    int velocity = 0;
    /// Pattern mode: inside the pattern, folded into 0..length-1. Song mode: song tick, not folded; up to
    /// 1/16 before the start of the recording.
    std::int64_t tick = 0;
};

struct PlayRequest
{
    int patternIndex = 0;
    std::uint32_t take = 0; ///< > 0 records under this number, 0 only plays
    int countInBars = 0;    ///< 0..2, only before a recording (F-TR-09)
    bool song = false;      ///< plays the song timeline instead of the pattern (F-TR-05)
};

/// Transport and pattern sequencer (F-TR-01 to 09, F-IN-07 to 10). In the song mode it plays the song
/// blocks of the snapshot one after another instead of one pattern (F-TR-05); loop, position and stop
/// at the end then refer to the whole song, which lasts at least until the backing track ends. A
/// recording in the song mode (F-BT-07) neither loops nor stops at the song end: it runs until Stop, and
/// its hits keep their song tick.
///
/// The GUI thread sends play, stop and rewind as commands through a lock-free queue and sets the
/// options through atomics; it reads the state and position back through atomics, 30 times a second
/// (Pflichtenheft chapter 3), and the recorded hits through a second queue.
///
/// The audio thread calls process() at the start of each block. Positions are kept as continuous
/// ticks since an anchor (the start, or the last tempo change), and every event is placed on the
/// sample floor((tick - anchor) * samplesPerTick) counted from the anchor. That puts each note on its
/// exact sample in the block (F-TR-06), plays it exactly once however the blocks are cut, and never
/// accumulates rounding errors (Q-06).
class Sequencer
{
public:
    static constexpr std::size_t kCommandQueueSize = 32;
    static constexpr std::size_t kRecordQueueSize = 1024;
    static constexpr int kMaxCountInBars = 2;

    // GUI thread
    /// Starts at the current position; ignored while running. False if the command queue is full.
    bool play(const PlayRequest& request) noexcept;
    /// Keeps the position for the next play().
    bool stop() noexcept;
    /// Back to the start of the pattern, also while running.
    bool rewind() noexcept;
    /// Moves the position to the tick, also while running (F-BT-07); while running it is folded into the
    /// pattern or song. Negative ticks count as 0.
    bool locate(std::int64_t tick) noexcept;
    /// Off: the transport stops at the end of the current pattern pass (F-TR-02).
    void setLoop(bool loop) noexcept;
    [[nodiscard]] bool loop() const noexcept;
    /// Clicks on every beat, separately while playing and while recording (F-TR-08).
    void setMetronome(bool onPlayback, bool onRecord) noexcept;
    /// Time from the start of a block to the moment its first sample is heard, plus any correction;
    /// recorded hits are moved back by it (F-IN-08).
    void setLatencyCompensation(double seconds) noexcept;
    [[nodiscard]] TransportState state() const noexcept;
    /// Tick inside the pattern; during the count-in the tick the recording starts at (F-TR-04).
    [[nodiscard]] std::int64_t position() const noexcept;
    /// Take of the running recording, 0 if none.
    [[nodiscard]] std::uint32_t activeTake() const noexcept;
    /// Tick the last recording started at, after the count-in.
    [[nodiscard]] std::int64_t recordStart() const noexcept;
    bool popRecordedHit(RecordedHit& hit) noexcept;

    // Audio thread
    void prepare(double sampleRate) noexcept;
    /// Applies the commands and fills the block with the notes and clicks of the next numSamples.
    /// backingSamples: device samples from the song start to the end of the backing track; the song
    /// lasts at least that long, rounded up to whole bars.
    void process(const ProjectSnapshot* snapshot,
                 int numSamples,
                 SequencerBlock& block,
                 std::int64_t backingSamples = 0) noexcept;
    /// Records the live hits the engine played in the block of the last process() call. blockTime is
    /// the Clock time at the start of that block.
    void record(std::span<const LiveHit> hits, double blockTimeSeconds) noexcept;

private:
    enum class CommandType : std::uint8_t
    {
        play,
        stop,
        locate
    };

    struct Command
    {
        CommandType type = CommandType::stop;
        PlayRequest request;
        std::int64_t tick = 0; ///< for locate
    };

    /// What plays: the pattern looped, or the song blocks one after another.
    struct Timeline
    {
        const ProjectSnapshot* snapshot = nullptr;
        const PatternSnapshot* pattern = nullptr; ///< pattern mode only
        bool song = false;
        std::int64_t length = 0;
        std::int64_t barTicks = 0;
        std::int64_t beatTicks = 0;
        std::int64_t sixteenthTicks = 0;
    };

    [[nodiscard]] Timeline timelineOf(const ProjectSnapshot& snapshot) const noexcept;
    void applyCommands(const ProjectSnapshot& snapshot) noexcept;
    void start(const PlayRequest& request, const ProjectSnapshot& snapshot) noexcept;
    void halt(const ProjectSnapshot& snapshot) noexcept;
    void locateTo(std::int64_t tick, const ProjectSnapshot& snapshot) noexcept;
    void updateTempo(const ProjectSnapshot& snapshot) noexcept;
    void schedule(const Timeline& timeline, int numSamples, SequencerBlock& block) noexcept;
    void scheduleNotes(const Timeline& timeline,
                       std::pair<std::int64_t, std::int64_t> range,
                       int numSamples,
                       SequencerBlock& block) const noexcept;
    void scheduleSong(const Timeline& timeline,
                      std::int64_t passStart,
                      std::pair<std::int64_t, std::int64_t> range,
                      int numSamples,
                      SequencerBlock& block) const noexcept;
    /// Notes of the pattern that start in range, played from segmentStart for segmentLength ticks.
    void scheduleSegment(const PatternSnapshot& pattern,
                         std::pair<std::int64_t, std::int64_t> segment,
                         std::pair<std::int64_t, std::int64_t> range,
                         int numSamples,
                         SequencerBlock& block) const noexcept;
    void scheduleClicks(const Timeline& timeline,
                        std::pair<std::int64_t, std::int64_t> range,
                        int numSamples,
                        SequencerBlock& block) const noexcept;
    void scheduleSongSpans(const Timeline& timeline, int numSamples, SequencerBlock& block) const noexcept;
    [[nodiscard]] std::int64_t songLength(const ProjectSnapshot& snapshot,
                                          std::int64_t barTicks) const noexcept;
    void advance(const Timeline& timeline, int numSamples) noexcept;
    void publish(std::int64_t patternLength) noexcept;

    [[nodiscard]] double tickAt(std::int64_t sampleInBlock) const noexcept;
    [[nodiscard]] std::int64_t sampleOffsetOf(std::int64_t tick) const noexcept;
    [[nodiscard]] std::int64_t patternTick(std::int64_t patternLength) const noexcept;
    [[nodiscard]] std::int64_t passEnd(std::int64_t patternLength) const noexcept;
    [[nodiscard]] bool metronomeOn() const noexcept;

    // Shared with the GUI thread (ordered by size, against padding)
    SpscQueue<Command, kCommandQueueSize> commands_;
    SpscQueue<RecordedHit, kRecordQueueSize> recorded_;
    std::atomic<double> latencySeconds_{0.0};
    std::atomic<std::int64_t> publishedPosition_{0};
    std::atomic<std::int64_t> publishedRecordStart_{0};
    std::atomic<std::uint32_t> publishedTake_{0};
    std::atomic<bool> loop_{true};
    std::atomic<bool> metronomeOnPlayback_{false};
    std::atomic<bool> metronomeOnRecord_{true};
    std::atomic<TransportState> publishedState_{TransportState::stopped};

    // Audio thread only
    double sampleRate_ = 48000.0;
    std::int64_t pausedTick_ = 0;
    std::int64_t countInEnd_ = 0; ///< tick the playback (and recording) starts at
    double anchorTick_ = 0.0;
    std::int64_t samplesSinceAnchor_ = 0;
    double samplesPerTick_ = 1.0;
    std::int64_t backingSamples_ = 0; ///< see process()
    int patternIndex_ = 0;
    std::uint32_t take_ = 0;
    TransportState state_ = TransportState::stopped;
    bool song_ = false;
    bool loopThisBlock_ = true;
    // What record() needs of the block process() last filled
    bool blockSong_ = false;
    std::uint32_t blockTake_ = 0;
    double blockStartTick_ = 0.0;
    std::int64_t blockLength_ = 1;
    std::int64_t blockTolerance_ = 0; ///< hits this early before the start still count
};

} // namespace drumprog::engine
