#pragma once

#include "engine/Metronome.h"
#include "engine/Sequencer.h"
#include "io/SettingsStore.h"
#include "model/TakeRecorder.h"
#include "ui/ITransportControl.h"

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <optional>
#include <string>

namespace drumprog::ui
{

/// What Play plays (F-TR-05).
enum class PlayMode : std::uint8_t
{
    pattern, ///< the active pattern
    song     ///< the song timeline
};

/// Logic of the transport bar (F-TR-01, 02, 04, 07 to 09) and of the recording into the active pattern
/// (F-IN-07 to 10). It sends commands to the sequencer, and its tick() takes the recorded hits from
/// the audio thread and writes them into the model through the TakeRecorder. GUI thread only.
class TransportPresenter final : public ITransportControl
{
public:
    static constexpr double kMaxRecordOffsetMs = 50.0;

    TransportPresenter(juce::ValueTree project,
                       engine::Sequencer& sequencer,
                       engine::Metronome& metronome,
                       model::TakeRecorder& recorder,
                       io::SettingsStore& offsetSettings);

    /// Records if Rec is armed, after the count-in.
    void play();
    void stop();
    void togglePlay() override;
    void rewind();
    [[nodiscard]] bool isPlaying() const;

    void setLoop(bool loop);
    [[nodiscard]] bool loop() const;

    void toggleRecordArmed();
    [[nodiscard]] bool isRecordArmed() const noexcept { return recordArmed_; }
    /// True from Play to Stop of a recording, count-in included: Rec lights red (F-TR-07).
    [[nodiscard]] bool isRecording() const;
    [[nodiscard]] bool isCountingIn() const;
    void setRecordMode(model::RecordMode mode) noexcept { recordMode_ = mode; }
    [[nodiscard]] model::RecordMode recordMode() const noexcept { return recordMode_; }

    /// 0, 1 or 2 bars (F-TR-09).
    void setCountInBars(int bars);
    [[nodiscard]] int countInBars() const noexcept { return countInBars_; }
    void setMetronomeOnPlayback(bool on);
    [[nodiscard]] bool metronomeOnPlayback() const noexcept { return metronomeOnPlayback_; }
    void setMetronomeOnRecord(bool on);
    [[nodiscard]] bool metronomeOnRecord() const noexcept { return metronomeOnRecord_; }
    /// Linear gain 0..1 (F-TR-08).
    void setMetronomeLevel(float level);
    [[nodiscard]] float metronomeLevel() const;

    /// Output latency of the audio device, subtracted from recorded hits (F-IN-08).
    void setOutputLatencyMs(double latencyMs);
    /// Extra correction of the recording, -50..+50 ms; positive moves notes later. Saved as setting.
    void setRecordOffsetMs(double offsetMs);
    [[nodiscard]] double recordOffsetMs() const noexcept { return recordOffsetMs_; }

    /// Pattern that plays and records; the pattern list chooses it (F-SO-01). A running playback
    /// continues in the new pattern; a running recording keeps its pattern.
    void setActivePattern(int patternIndex);
    [[nodiscard]] int activePattern() const noexcept { return activePattern_; }

    /// Switching restarts a running playback from the start in the other mode. Recording only works in
    /// the pattern mode, so a running recording keeps the mode until Stop.
    void setPlayMode(PlayMode mode);
    [[nodiscard]] PlayMode playMode() const noexcept { return playMode_; }

    /// Bar.Beat.Tick of the playback position, e.g. "003.2.090" (F-TR-04).
    [[nodiscard]] std::string positionText() const;

    /// Tick of the playback cursor in the active pattern, nothing while stopped (F-PR-11). In the song
    /// mode only while a block of the active pattern plays.
    [[nodiscard]] std::optional<std::int64_t> playheadTick() const;
    /// Tick of the playback cursor in the song, nothing while stopped or in the pattern mode (F-PR-11).
    [[nodiscard]] std::optional<std::int64_t> songPlayheadTick() const;

    /// Call from the GUI timer: writes recorded hits into the pattern and ends a finished recording.
    void tick();

private:
    void restart();
    void writeRecordedHits();
    void endTake();
    void updateLatencyCompensation();
    void loadRecordOffset();

    juce::ValueTree project_;
    engine::Sequencer& sequencer_;
    engine::Metronome& metronome_;
    model::TakeRecorder& recorder_;
    io::SettingsStore& offsetSettings_;
    bool recordArmed_ = false;
    model::RecordMode recordMode_ = model::RecordMode::overdub;
    int countInBars_ = 1;
    bool metronomeOnPlayback_ = false;
    bool metronomeOnRecord_ = true;
    double outputLatencyMs_ = 0.0;
    double recordOffsetMs_ = 0.0;
    int activePattern_ = 0;
    PlayMode playMode_ = PlayMode::pattern;
    std::uint32_t nextTake_ = 1;
    std::uint32_t take_ = 0; ///< recording that was started, 0 if none
    bool takeStarted_ = false;
};

} // namespace drumprog::ui
