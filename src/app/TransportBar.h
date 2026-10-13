#pragma once

#include "app/IconButton.h"
#include "app/Theme.h"
#include "i18n/Translate.h"
#include "ui/TempoPresenter.h"
#include "ui/TransportPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drumprog::app
{

/// Humble object (E-03): transport bar (Pflichtenheft 6.1, area 1) in one row with back, play, stop, loop
/// and rec as icon buttons, the highlighted position, BPM and time signature and the mode Pattern/Song
/// (F-TR-05). Metronome and recording settings are in MetronomePanel, the recording mode in
/// PianoRollToolbar (AP14). All logic is in ui::TransportPresenter and ui::TempoPresenter; a 30 Hz timer
/// refreshes the display and hands recorded hits to the model (F-TR-04).
class TransportBar final : public juce::Component, private juce::Timer
{
public:
    TransportBar(ui::TransportPresenter& transport, ui::TempoPresenter& tempo);
    ~TransportBar() override;

    TransportBar(const TransportBar&) = delete;
    TransportBar& operator=(const TransportBar&) = delete;
    TransportBar(TransportBar&&) = delete;
    TransportBar& operator=(TransportBar&&) = delete;

    static constexpr int kPreferredHeight = 30;

    void resized() override;
    /// Takes the colours of a new theme (AP16).
    void lookAndFeelChanged() override;

private:
    void timerCallback() override;
    void setUpTransport();
    void setUpTempo();
    void setUpMode();
    void refresh();
    void refreshTempo();
    void commitBpm();
    void commitTimeSignature();

    ui::TransportPresenter& transport_;
    ui::TempoPresenter& tempo_;
    int blink_ = 0;

    IconButton rewindButton_{Icon::rewind, i18n::trJuce("Back to the start")};
    IconButton playButton_{Icon::play, i18n::trJuce("Play (space bar)")};
    IconButton stopButton_{Icon::stop, i18n::trJuce("Stop (space bar)")};
    IconButton loopButton_{Icon::loop, "Loop"};
    IconButton recButton_{Icon::record, i18n::trJuce("Arm recording (Ctrl+R)")};
    theme::DimLabel positionTitle_{{}, i18n::trJuce("Position")};
    juce::Label positionLabel_{{}, "001.1.000"};
    theme::DimLabel bpmLabel_{{}, "BPM"};
    juce::TextEditor bpmEditor_;
    theme::DimLabel signatureLabel_{{}, i18n::trJuce("Time")};
    juce::ComboBox numeratorBox_;
    juce::ComboBox denominatorBox_;
    theme::DimLabel modeLabel_{{}, i18n::trJuce("Mode")};
    IconButton patternModeButton_{Icon::patternMode, i18n::trJuce("Pattern mode: plays the active pattern")};
    IconButton songModeButton_{Icon::songMode, i18n::trJuce("Song mode: plays the song timeline")};
};

} // namespace drumprog::app
