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
/// Length of the song while it records: it never loops or ends by itself.
constexpr std::int64_t kEndlessTicks = std::numeric_limits<std::int64_t>::max() / 4;

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
    return locate(0);
}

bool Sequencer::locate(std::int64_t tick) noexcept
{
    return commands_.push({CommandType::locate, {}, tick});
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

std::int64_t Sequencer::recordStart() const noexcept
{
    return publishedRecordStart_.load();
}

bool Sequencer::popRecordedHit(RecordedHit& hit) noexcept
{
    return recorded_.pop(hit);
}

void Sequencer::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
}

void Sequencer::process(const ProjectSnapshot* snapshot,
                        int numSamples,
                        SequencerBlock& block,
                        std::int64_t backingSamples) noexcept
{
    block.numNotes = 0;
    block.numClicks = 0;
    block.numSongSpans = 0;
    blockTake_ = 0;
    backingSamples_ = backingSamples;
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
        const std::int64_t rounded = std::llround(tick);
        recorded_.push({blockTake_,
                        hit.slotIndex,
                        hit.velocity,
                        blockSong_ ? rounded : wrapTick(rounded, blockLength_)});
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
        timeline.length = take_ != 0 ? kEndlessTicks : songLength(snapshot, timeline.barTicks);
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
            locateTo(command.tick, snapshot);
    }
}

void Sequencer::start(const PlayRequest& request, const ProjectSnapshot& snapshot) noexcept
{
    if (state_ != TransportState::stopped)
        return;
    patternIndex_ = request.patternIndex;
    song_ = request.song;
    take_ = request.take;
    const Timeline timeline = timelineOf(snapshot);
    const int countInBars = take_ != 0 ? std::clamp(request.countInBars, 0, kMaxCountInBars) : 0;
    countInEnd_ = wrapTick(pausedTick_, timeline.length);
    anchorTick_ = static_cast<double>(countInEnd_ - countInBars * timeline.barTicks);
    samplesSinceAnchor_ = 0;
    samplesPerTick_ = samplesPerTick(snapshot.bpm, snapshot.ticksPerQuarter, sampleRate_);
    state_ = countInBars > 0 ? TransportState::countIn : TransportState::playing;
    publishedRecordStart_.store(countInEnd_);
}

void Sequencer::halt(const ProjectSnapshot& snapshot) noexcept
{
    if (state_ == TransportState::stopped)
        return;
    pausedTick_ = patternTick(timelineOf(snapshot).length);
    state_ = TransportState::stopped;
    take_ = 0;
}

void Sequencer::locateTo(std::int64_t tick, const ProjectSnapshot& snapshot) noexcept
{
    const std::int64_t target = std::max<std::int64_t>(tick, 0);
    if (state_ == TransportState::stopped)
    {
        pausedTick_ = target;
        return;
    }
    // A running count-in keeps the time it has left.
    const double countInLeft = std::max(0.0, static_cast<double>(countInEnd_) - tickAt(0));
    countInEnd_ = wrapTick(target, timelineOf(snapshot).length);
    anchorTick_ = static_cast<double>(countInEnd_) - countInLeft;
    samplesSinceAnchor_ = 0;
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
    blockSong_ = timeline.song;
    // One tick of margin on both sides; sampleOffsetOf() decides exactly.
    const std::pair range{static_cast<std::int64_t>(std::floor(blockStartTick_)) - 1,
                          static_cast<std::int64_t>(std::ceil(tickAt(numSamples))) + 1};
    scheduleNotes(timeline, range, numSamples, block);
    scheduleClicks(timeline, range, numSamples, block);
    if (timeline.song)
        scheduleSongSpans(timeline, numSamples, block);
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

void Sequencer::scheduleSongSpans(const Timeline& timeline,
                                  int numSamples,
                                  SequencerBlock& block) const noexcept
{
    const std::int64_t end = passEnd(timeline.length);
    const auto tick = static_cast<std::int64_t>(std::floor(tickAt(0)));
    // The count-in belongs to the pass it leads into, so it plays the song before its start.
    std::int64_t pass = floorDiv(std::max(tick, countInEnd_), timeline.length);
    for (std::int64_t from = 0; from < numSamples && block.numSongSpans < SequencerBlock::kMaxSongSpans;
         ++pass)
    {
        const std::int64_t passStart = pass * timeline.length;
        const std::int64_t passStop = std::min(passStart + timeline.length, end);
        // The endless song of a recording ends far beyond any block.
        const bool endsInBlock = static_cast<double>(passStop) <= tickAt(numSamples) + 1.0;
        const std::int64_t to =
            endsInBlock ? std::clamp<std::int64_t>(sampleOffsetOf(passStop), from, numSamples) : numSamples;
        if (to > from)
        {
            const auto anchorSample = static_cast<std::int64_t>(
                std::floor((anchorTick_ - static_cast<double>(passStart)) * samplesPerTick_));
            block.songSpans.at(block.numSongSpans++) = {static_cast<int>(from),
                                                        static_cast<int>(to - from),
                                                        anchorSample + samplesSinceAnchor_ + from};
        }
        if (passStop >= end)
            break; // without loop the song ends here
        from = to;
    }
}

std::int64_t Sequencer::songLength(const ProjectSnapshot& snapshot, std::int64_t barTicks) const noexcept
{
    std::int64_t length = std::max(snapshot.songLengthTicks, barTicks);
    if (backingSamples_ > 0)
    {
        const double ticks = static_cast<double>(backingSamples_) /
                             samplesPerTick(snapshot.bpm, snapshot.ticksPerQuarter, sampleRate_);
        const auto bars = static_cast<std::int64_t>(std::ceil(ticks / static_cast<double>(barTicks)));
        length = std::max(length, bars * barTicks);
    }
    return length;
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
