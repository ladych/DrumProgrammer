#include "ui/TransportPresenter.h"

#include "model/Project.h"
#include "model/SongLayout.h"
#include "ui/MusicalTime.h"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <utility>

namespace drumprog::ui
{
namespace
{

constexpr double kMillisecondsPerSecond = 1000.0;

} // namespace

TransportPresenter::TransportPresenter(juce::ValueTree project,
                                       engine::Sequencer& sequencer,
                                       engine::Metronome& metronome,
                                       model::TakeRecorder& recorder,
                                       model::SongTakeRecorder& songRecorder,
                                       io::SettingsStore& offsetSettings)
    : project_(std::move(project)), sequencer_(sequencer), metronome_(metronome), recorder_(recorder),
      songRecorder_(songRecorder), offsetSettings_(offsetSettings)
{
    sequencer_.setMetronome(metronomeOnPlayback_, metronomeOnRecord_);
    loadRecordOffset();
    updateLatencyCompensation();
}

void TransportPresenter::play()
{
    if (isPlaying())
        return;
    const bool song = playMode_ == PlayMode::song;
    std::uint32_t take = 0;
    if (recordArmed_ && (song || recorder_.begin(activePattern_, recordMode_)))
    {
        if (song)
            songRecorder_.begin();
        take = nextTake_++;
        take_ = take;
        takeStarted_ = false;
        songTake_ = song;
    }
    sequencer_.play(
        {.patternIndex = activePattern_, .take = take, .countInBars = countInBars_, .song = song});
}

void TransportPresenter::stop()
{
    sequencer_.stop();
    if (take_ != 0)
        endTake();
}

void TransportPresenter::togglePlay()
{
    if (isPlaying())
        stop();
    else
        play();
}

void TransportPresenter::rewind()
{
    if (!songTake_)
        sequencer_.rewind();
}

void TransportPresenter::locate(std::int64_t songTick)
{
    if (take_ != 0)
        return;
    if (playMode_ == PlayMode::song)
    {
        sequencer_.locate(songTick);
        return;
    }
    playMode_ = PlayMode::song;
    restart();
    sequencer_.locate(songTick);
}

bool TransportPresenter::isPlaying() const
{
    return sequencer_.state() != engine::TransportState::stopped;
}

void TransportPresenter::setLoop(bool loop)
{
    sequencer_.setLoop(loop);
}

bool TransportPresenter::loop() const
{
    return sequencer_.loop();
}

void TransportPresenter::toggleRecordArmed()
{
    recordArmed_ = !recordArmed_;
}

bool TransportPresenter::isRecording() const
{
    return take_ != 0 && sequencer_.activeTake() == take_;
}

bool TransportPresenter::isCountingIn() const
{
    return sequencer_.state() == engine::TransportState::countIn;
}

void TransportPresenter::setCountInBars(int bars)
{
    countInBars_ = std::clamp(bars, 0, engine::Sequencer::kMaxCountInBars);
}

void TransportPresenter::setMetronomeOnPlayback(bool on)
{
    metronomeOnPlayback_ = on;
    sequencer_.setMetronome(metronomeOnPlayback_, metronomeOnRecord_);
}

void TransportPresenter::setMetronomeOnRecord(bool on)
{
    metronomeOnRecord_ = on;
    sequencer_.setMetronome(metronomeOnPlayback_, metronomeOnRecord_);
}

void TransportPresenter::setMetronomeLevel(float level)
{
    metronome_.setLevel(level);
}

float TransportPresenter::metronomeLevel() const
{
    return metronome_.level();
}

void TransportPresenter::setOutputLatencyMs(double latencyMs)
{
    outputLatencyMs_ = latencyMs;
    updateLatencyCompensation();
}

void TransportPresenter::setRecordOffsetMs(double offsetMs)
{
    recordOffsetMs_ = std::clamp(offsetMs, -kMaxRecordOffsetMs, kMaxRecordOffsetMs);
    updateLatencyCompensation();
    std::ostringstream text;
    text << recordOffsetMs_;
    offsetSettings_.save(text.str());
}

std::string TransportPresenter::positionText() const
{
    const model::Project project{project_, nullptr};
    return formatPosition(sequencer_.position(), project.ticksPerQuarter(), project.timeSignature());
}

std::optional<std::int64_t> TransportPresenter::playheadTick() const
{
    if (!isPlaying())
        return std::nullopt;
    const std::int64_t position = sequencer_.position();
    if (playMode_ == PlayMode::pattern)
        return position;
    for (const auto& block : model::layoutSong(model::Project{project_, nullptr}))
    {
        const std::int64_t tick = position - block.startTick;
        if (block.patternIndex == activePattern_ && tick >= 0 && tick < block.playedTicks)
            return tick;
    }
    return std::nullopt;
}

std::optional<std::int64_t> TransportPresenter::songPlayheadTick() const
{
    if (playMode_ != PlayMode::song)
        return std::nullopt;
    return sequencer_.position();
}

void TransportPresenter::setPlayMode(PlayMode mode)
{
    if (mode == playMode_ || take_ != 0)
        return;
    playMode_ = mode;
    restart();
}

void TransportPresenter::restart()
{
    // The sequencer still runs until the audio thread takes the commands, so play() would ignore it.
    const bool playing = isPlaying();
    sequencer_.stop();
    sequencer_.rewind();
    if (playing)
        sequencer_.play({.patternIndex = activePattern_, .song = playMode_ == PlayMode::song});
}

void TransportPresenter::setActivePattern(int patternIndex)
{
    if (patternIndex == activePattern_)
        return;
    activePattern_ = patternIndex;
    // A running playback switches to the new pattern; a recording keeps its pattern until Stop, and the
    // song plays on.
    if (isPlaying() && take_ == 0 && playMode_ == PlayMode::pattern)
    {
        sequencer_.stop();
        sequencer_.play({.patternIndex = activePattern_});
    }
}

void TransportPresenter::tick()
{
    // Read the take before the hits: hits the audio thread recorded before ending it are then queued.
    const std::uint32_t activeTake = sequencer_.activeTake();
    writeRecordedHits();
    if (take_ == 0)
        return;
    if (activeTake == take_)
        takeStarted_ = true;
    else if (takeStarted_)
        endTake(); // stopped at the pattern end
}

void TransportPresenter::writeRecordedHits()
{
    engine::RecordedHit hit;
    while (sequencer_.popRecordedHit(hit))
    {
        if (take_ == 0 || hit.take != take_)
            continue;
        if (songTake_)
            songRecorder_.add(hit.slotIndex, hit.velocity, hit.tick);
        else
            recorder_.add(hit.slotIndex, hit.velocity, hit.tick);
    }
}

void TransportPresenter::endTake()
{
    writeRecordedHits();
    if (songTake_)
        endSongTake();
    else
        recorder_.end();
    take_ = 0;
    takeStarted_ = false;
    songTake_ = false;
}

void TransportPresenter::endSongTake()
{
    const auto pattern = songRecorder_.end(sequencer_.recordStart(), sequencer_.position());
    if (pattern && onSongTake_)
        onSongTake_(*pattern);
}

void TransportPresenter::updateLatencyCompensation()
{
    sequencer_.setLatencyCompensation((outputLatencyMs_ - recordOffsetMs_) / kMillisecondsPerSecond);
}

void TransportPresenter::loadRecordOffset()
{
    const auto saved = offsetSettings_.load();
    if (!saved)
        return;
    double value = 0.0;
    const auto* const end = saved->data() + saved->size();
    if (std::from_chars(saved->data(), end, value).ec == std::errc{})
        recordOffsetMs_ = std::clamp(value, -kMaxRecordOffsetMs, kMaxRecordOffsetMs);
}

} // namespace drumprog::ui
