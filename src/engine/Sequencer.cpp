#include "engine/Sequencer.h"

#include "engine/TempoMath.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drumprog::engine
{
namespace
{

constexpr int kSixteenthsPerQuarter = 4;

void addNote(SequencerBlock& block, const SequencedNote& note) noexcept
{
    if (block.numNotes < SequencerBlock::kMaxNotes)
        block.notes.at(block.numNotes++) = note;
}

void addClick(SequencerBlock& block, const MetronomeClick& click) noexcept
{
    if (block.numClicks < SequencerBlock::kMaxClicks)
        block.clicks.at(block.numClicks++) = click;
}

} // namespace

bool Sequencer::play(const PlayRequest& request) noexcept
{
    return commands_.push({CommandType::play, request});
}

bool Sequencer::stop() noexcept
{
    return commands_.push({CommandType::stop, {}});
}

bool Sequencer::rewind() noexcept
{
    return commands_.push({CommandType::rewind, {}});
}

void Sequencer::setLoop(bool loop) noexcept
{
    loop_.store(loop);
}

bool Sequencer::loop() const noexcept
{
    return loop_.load();
}

void Sequencer::setMetronome(bool onPlayback, bool onRecord) noexcept
{
    metronomeOnPlayback_.store(onPlayback);
    metronomeOnRecord_.store(onRecord);
}

void Sequencer::setLatencyCompensation(double seconds) noexcept
{
    latencySeconds_.store(seconds);
}

TransportState Sequencer::state() const noexcept
{
    return publishedState_.load();
}

std::int64_t Sequencer::position() const noexcept
{
    return publishedPosition_.load();
}

std::uint32_t Sequencer::activeTake() const noexcept
{
    return publishedTake_.load();
}

bool Sequencer::popRecordedHit(RecordedHit& hit) noexcept
{
    return recorded_.pop(hit);
}

void Sequencer::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
}

void Sequencer::process(const ProjectSnapshot* snapshot, int numSamples, SequencerBlock& block) noexcept
{
    block.numNotes = 0;
    block.numClicks = 0;
    blockTake_ = 0;
    if (snapshot == nullptr)
        return;
    applyCommands(*snapshot);
    const Timeline timeline = timelineOf(*snapshot);
    if (state_ != TransportState::stopped)
    {
        updateTempo(*snapshot);
        schedule(timeline, numSamples, block);
        advance(timeline, numSamples);
    }
    publish(timeline.length);
}

void Sequencer::record(std::span<const LiveHit> hits, double blockTimeSeconds) noexcept
{
    if (blockTake_ == 0)
        return;
    const double ticksPerSecond = sampleRate_ / samplesPerTick_;
    const double latency = latencySeconds_.load();
    for (const auto& hit : hits)
    {
        // The hit was played to what was heard at its time: the block start, moved back by the hit's
        // age and by the time until a block is heard (F-IN-08).
        const double tick = blockStartTick_ - (blockTimeSeconds - hit.timeSeconds + latency) * ticksPerSecond;
        if (tick < static_cast<double>(countInEnd_ - blockTolerance_))
            continue; // played during the count-in
        recorded_.push({blockTake_, hit.slotIndex, hit.velocity, wrapTick(std::llround(tick), blockLength_)});
    }
}

Sequencer::Timeline Sequencer::timelineOf(const ProjectSnapshot& snapshot) const noexcept
{
    Timeline timeline;
    timeline.snapshot = &snapshot;
    timeline.song = song_;
    timeline.barTicks =
        ticksPerBar(snapshot.ticksPerQuarter, snapshot.timeSigNumerator, snapshot.timeSigDenominator);
    timeline.beatTicks = ticksPerBeat(snapshot.ticksPerQuarter, snapshot.timeSigDenominator);
    timeline.sixteenthTicks = snapshot.ticksPerQuarter / kSixteenthsPerQuarter;
    timeline.length = timeline.barTicks;
    if (song_)
        timeline.length = std::max(snapshot.songLengthTicks, timeline.barTicks);
    else if (patternIndex_ >= 0 && static_cast<std::size_t>(patternIndex_) < snapshot.patterns.size())
    {
        timeline.pattern = &snapshot.patterns[static_cast<std::size_t>(patternIndex_)];
        timeline.length = std::max(timeline.pattern->lengthTicks, timeline.barTicks);
    }
    return timeline;
}

void Sequencer::applyCommands(const ProjectSnapshot& snapshot) noexcept
{
    Command command;
    while (commands_.pop(command))
    {
        if (command.type == CommandType::play)
            start(command.request, snapshot);
        else if (command.type == CommandType::stop)
            halt(snapshot);
        else
            rewindToStart();
    }
}

void Sequencer::start(const PlayRequest& request, const ProjectSnapshot& snapshot) noexcept
{
    if (state_ != TransportState::stopped)
        return;
    patternIndex_ = request.patternIndex;
    song_ = request.song;
    take_ = song_ ? 0 : request.take;
    const Timeline timeline = timelineOf(snapshot);
    const int countInBars = take_ != 0 ? std::clamp(request.countInBars, 0, kMaxCountInBars) : 0;
    countInEnd_ = wrapTick(pausedTick_, timeline.length);
    anchorTick_ = static_cast<double>(countInEnd_ - countInBars * timeline.barTicks);
    samplesSinceAnchor_ = 0;
    samplesPerTick_ = samplesPerTick(snapshot.bpm, snapshot.ticksPerQuarter, sampleRate_);
    state_ = countInBars > 0 ? TransportState::countIn : TransportState::playing;
}

void Sequencer::halt(const ProjectSnapshot& snapshot) noexcept
{
    if (state_ == TransportState::stopped)
        return;
    pausedTick_ = patternTick(timelineOf(snapshot).length);
    state_ = TransportState::stopped;
    take_ = 0;
}

void Sequencer::rewindToStart() noexcept
{
    if (state_ == TransportState::stopped)
    {
        pausedTick_ = 0;
        return;
    }
    // A running count-in keeps the time it has left.
    const double countInLeft = std::max(0.0, static_cast<double>(countInEnd_) - tickAt(0));
    anchorTick_ = -countInLeft;
    samplesSinceAnchor_ = 0;
    countInEnd_ = 0;
}

void Sequencer::updateTempo(const ProjectSnapshot& snapshot) noexcept
{
    const double samplesPerTickNow = samplesPerTick(snapshot.bpm, snapshot.ticksPerQuarter, sampleRate_);
    if (samplesPerTickNow == samplesPerTick_)
        return;
    // New anchor at the current position, so the tempo change does not move it.
    anchorTick_ = tickAt(0);
    samplesSinceAnchor_ = 0;
    samplesPerTick_ = samplesPerTickNow;
}

void Sequencer::schedule(const Timeline& timeline, int numSamples, SequencerBlock& block) noexcept
{
    loopThisBlock_ = loop_.load();
    blockTake_ = take_;
    blockStartTick_ = tickAt(0);
    blockLength_ = timeline.length;
    blockTolerance_ = timeline.sixteenthTicks;
    // One tick of margin on both sides; sampleOffsetOf() decides exactly.
    const std::pair range{static_cast<std::int64_t>(std::floor(blockStartTick_)) - 1,
                          static_cast<std::int64_t>(std::ceil(tickAt(numSamples))) + 1};
    scheduleNotes(timeline, range, numSamples, block);
    scheduleClicks(timeline, range, numSamples, block);
}

void Sequencer::scheduleNotes(const Timeline& timeline,
                              std::pair<std::int64_t, std::int64_t> range,
                              int numSamples,
                              SequencerBlock& block) const noexcept
{
    const std::int64_t from = std::max(range.first, countInEnd_);
    const std::int64_t to = std::min(range.second, passEnd(timeline.length) - 1);
    for (std::int64_t pass = floorDiv(from, timeline.length); pass <= floorDiv(to, timeline.length); ++pass)
    {
        const std::int64_t passStart = pass * timeline.length;
        if (timeline.song)
            scheduleSong(timeline, passStart, {from, to}, numSamples, block);
        else if (timeline.pattern != nullptr)
            scheduleSegment(*timeline.pattern, {passStart, timeline.length}, {from, to}, numSamples, block);
    }
}

void Sequencer::scheduleSong(const Timeline& timeline,
                             std::int64_t passStart,
                             std::pair<std::int64_t, std::int64_t> range,
                             int numSamples,
                             SequencerBlock& block) const noexcept
{
    const auto& patterns = timeline.snapshot->patterns;
    for (const auto& entry : timeline.snapshot->song)
    {
        const std::int64_t start = passStart + entry.startTick;
        if (start > range.second)
            break;
        if (entry.patternIndex < 0 || static_cast<std::size_t>(entry.patternIndex) >= patterns.size())
            continue;
        scheduleSegment(patterns[static_cast<std::size_t>(entry.patternIndex)],
                        {start, entry.lengthTicks},
                        range,
                        numSamples,
                        block);
    }
}

void Sequencer::scheduleSegment(const PatternSnapshot& pattern,
                                std::pair<std::int64_t, std::int64_t> segment,
                                std::pair<std::int64_t, std::int64_t> range,
                                int numSamples,
                                SequencerBlock& block) const noexcept
{
    const auto [start, length] = segment;
    const auto& notes = pattern.notes;
    auto note = std::ranges::lower_bound(notes, range.first - start, {}, &NoteSnapshot::startTick);
    for (; note != notes.end() && note->startTick <= range.second - start && note->startTick < length; ++note)
    {
        const std::int64_t offset = sampleOffsetOf(start + note->startTick);
        if (offset >= 0 && offset < numSamples)
            addNote(block, {note->slotIndex, note->velocity, static_cast<int>(offset)});
    }
}

void Sequencer::scheduleClicks(const Timeline& timeline,
                               std::pair<std::int64_t, std::int64_t> range,
                               int numSamples,
                               SequencerBlock& block) const noexcept
{
    const std::int64_t to = std::min(range.second, passEnd(timeline.length) - 1);
    const bool metronome = metronomeOn();
    for (std::int64_t beat = floorDiv(range.first, timeline.beatTicks) * timeline.beatTicks; beat <= to;
         beat += timeline.beatTicks)
    {
        const std::int64_t offset = sampleOffsetOf(beat);
        // The count-in always clicks (F-TR-09), the metronome only when switched on (F-TR-08).
        if (offset < 0 || offset >= numSamples || (beat >= countInEnd_ && !metronome))
            continue;
        addClick(block, {wrapTick(beat, timeline.barTicks) == 0, static_cast<int>(offset)});
    }
}

void Sequencer::advance(const Timeline& timeline, int numSamples) noexcept
{
    const std::int64_t end = passEnd(timeline.length);
    samplesSinceAnchor_ += numSamples;
    if (tickAt(0) >= static_cast<double>(end))
    {
        // End of the pattern without loop: stop and start from the beginning next time.
        state_ = TransportState::stopped;
        take_ = 0;
        pausedTick_ = 0;
    }
    else if (tickAt(0) >= static_cast<double>(countInEnd_))
        state_ = TransportState::playing;
}

void Sequencer::publish(std::int64_t patternLength) noexcept
{
    const bool stopped = state_ == TransportState::stopped;
    publishedPosition_.store(stopped ? pausedTick_ : patternTick(patternLength));
    publishedTake_.store(stopped ? 0 : take_);
    publishedState_.store(state_);
}

double Sequencer::tickAt(std::int64_t sampleInBlock) const noexcept
{
    return anchorTick_ + static_cast<double>(samplesSinceAnchor_ + sampleInBlock) / samplesPerTick_;
}

std::int64_t Sequencer::sampleOffsetOf(std::int64_t tick) const noexcept
{
    const double sample = (static_cast<double>(tick) - anchorTick_) * samplesPerTick_;
    return static_cast<std::int64_t>(std::floor(sample)) - samplesSinceAnchor_;
}

std::int64_t Sequencer::patternTick(std::int64_t patternLength) const noexcept
{
    const auto tick = static_cast<std::int64_t>(std::floor(tickAt(0)));
    return wrapTick(std::max(tick, countInEnd_), patternLength);
}

std::int64_t Sequencer::passEnd(std::int64_t patternLength) const noexcept
{
    if (loopThisBlock_)
        return std::numeric_limits<std::int64_t>::max();
    const auto tick = static_cast<std::int64_t>(std::floor(tickAt(0)));
    return (floorDiv(std::max(tick, countInEnd_), patternLength) + 1) * patternLength;
}

bool Sequencer::metronomeOn() const noexcept
{
    return take_ != 0 ? metronomeOnRecord_.load() : metronomeOnPlayback_.load();
}

} // namespace drumprog::engine
