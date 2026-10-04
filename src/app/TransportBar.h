#pragma once

#include "ui/TempoPresenter.h"
#include "ui/TransportPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drumprog::app
{

/// Humble object (E-03): transport bar (Pflichtenheft 6.1, area 1) with back, play, stop, loop and
/// rec, the position, BPM and time signature, the mode Pattern/Song (F-TR-05), metronome, count-in, recording
/// mode and the recording offset. All logic is in ui::TransportPresenter and ui::TempoPresenter; a 30 Hz
/// timer refreshes the display and hands recorded hits to the model (F-TR-04).
class TransportBar final : public juce::Component, private juce::Timer
{
public:
    TransportBar(ui::TransportPresenter& transport, ui::TempoPresenter& tempo);
    ~TransportBar() override;

    TransportBar(const TransportBar&) = delete;
    TransportBar& operator=(const TransportBar&) = delete;
    TransportBar(TransportBar&&) = delete;
    TransportBar& operator=(TransportBar&&) = delete;

    static constexpr int kPreferredHeight = 72;

    void resized() override;

private:
    void timerCallback() override;
    void setUpTransport();
    void setUpTempo();
    void setUpMode();
    void layOutMode(juce::Rectangle<int> row);
    void layOutMetronomeAndRecording(juce::Rectangle<int> row);
    void setUpMetronome();
    void setUpRecording();
    void refresh();
    void refreshTempo();
    void commitBpm();
    void commitTimeSignature();

    ui::TransportPresenter& transport_;
    ui::TempoPresenter& tempo_;
    int blink_ = 0;

    juce::TextButton rewindButton_{"|<"};
    juce::TextButton playButton_{"Play"};
    juce::TextButton stopButton_{"Stop"};
    juce::TextButton loopButton_{"Loop"};
    juce::TextButton recButton_{"Rec"};
    juce::Label positionLabel_{{}, "001.1.000"};
    juce::Label bpmLabel_{{}, "BPM"};
    juce::TextEditor bpmEditor_;
    juce::Label signatureLabel_{{}, "Takt"};
    juce::ComboBox numeratorBox_;
    juce::ComboBox denominatorBox_;
    juce::Label modeLabel_{{}, "Modus"};
    juce::TextButton patternModeButton_{"Pattern"};
    juce::TextButton songModeButton_{"Song"};

    juce::ToggleButton metronomeButton_{"Metronom"};
    juce::ToggleButton metronomeRecordButton_{"bei Aufnahme"};
    juce::Slider metronomeLevel_{juce::Slider::LinearHorizontal, juce::Slider::NoTextBox};
    juce::ComboBox countInBox_;
    juce::ComboBox recordModeBox_;
    juce::Label offsetLabel_{{}, "Versatz (ms)"};
    juce::Slider offsetSlider_{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
};

} // namespace drumprog::app
