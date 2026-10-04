#include "ui/TransportPresenter.h"

#include "engine/TempoMath.h"
#include "model/Project.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
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
                                       io::SettingsStore& offsetSettings)
    : project_(std::move(project)), sequencer_(sequencer), metronome_(metronome), recorder_(recorder),
      offsetSettings_(offsetSettings)
{
    sequencer_.setMetronome(metronomeOnPlayback_, metronomeOnRecord_);
    loadRecordOffset();
    updateLatencyCompensation();
}

void TransportPresenter::play()
{
    if (isPlaying())
        return;
    std::uint32_t take = 0;
    if (recordArmed_ && recorder_.begin(activePattern_, recordMode_))
    {
        take = nextTake_++;
        take_ = take;
        takeStarted_ = false;
    }
    sequencer_.play({.patternIndex = activePattern_, .take = take, .countInBars = countInBars_});
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
    sequencer_.rewind();
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
    const auto signature = project.timeSignature();
    const std::int64_t beatTicks = engine::ticksPerBeat(project.ticksPerQuarter(), signature.denominator);
    const std::int64_t barTicks = beatTicks * signature.numerator;
    const std::int64_t position = sequencer_.position();
    std::ostringstream text;
    text << std::setfill('0') << std::setw(3) << position / barTicks + 1 << '.'
         << position % barTicks / beatTicks + 1 << '.' << std::setw(3) << position % beatTicks;
    return text.str();
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
        if (take_ != 0 && hit.take == take_)
            recorder_.add(hit.slotIndex, hit.velocity, hit.tick);
}

void TransportPresenter::endTake()
{
    writeRecordedHits();
    recorder_.end();
    take_ = 0;
    takeStarted_ = false;
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
