#pragma once

#include "app/IconButton.h"
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

    IconButton rewindButton_{Icon::rewind, juce::String::fromUTF8("Zurück zum Anfang")};
    IconButton playButton_{Icon::play, "Wiedergabe (Leertaste)"};
    IconButton stopButton_{Icon::stop, "Stopp (Leertaste)"};
    IconButton loopButton_{Icon::loop, "Loop"};
    IconButton recButton_{Icon::record, "Aufnahme scharf schalten (Strg+R)"};
    juce::Label positionTitle_{{}, "Position"};
    juce::Label positionLabel_{{}, "001.1.000"};
    juce::Label bpmLabel_{{}, "BPM"};
    juce::TextEditor bpmEditor_;
    juce::Label signatureLabel_{{}, "Takt"};
    juce::ComboBox numeratorBox_;
    juce::ComboBox denominatorBox_;
    juce::Label modeLabel_{{}, "Modus"};
    IconButton patternModeButton_{Icon::patternMode, "Modus Pattern: spielt das aktive Pattern"};
    IconButton songModeButton_{Icon::songMode, "Modus Song: spielt die Song-Timeline"};
};

} // namespace drumprog::app
