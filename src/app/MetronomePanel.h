#pragma once

#include "ui/TransportPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drumprog::app
{

/// Humble object (E-03): metronome and recording settings in the right column next to the song
/// arrangement (AP14), one below the other: metronome on playback and on recording, its level, the
/// count-in and the recording offset (F-TR-06, F-TR-07). Logic in ui::TransportPresenter.
class MetronomePanel final : public juce::Component
{
public:
    static constexpr int kMinimumHeight = 156;

    explicit MetronomePanel(ui::TransportPresenter& transport);

    void resized() override;

private:
    void setUpMetronome();
    void setUpRecording();

    ui::TransportPresenter& transport_;
    juce::Label title_{{}, "Metronom und Aufnahme"};
    juce::ToggleButton metronomeButton_{"Metronom"};
    juce::ToggleButton metronomeRecordButton_{"bei Aufnahme"};
    juce::Label levelLabel_{{}, "Pegel"};
    juce::Slider levelSlider_{juce::Slider::LinearHorizontal, juce::Slider::NoTextBox};
    juce::Label countInLabel_{{}, juce::String::fromUTF8("Vorzähler")};
    juce::ComboBox countInBox_;
    juce::Label offsetLabel_{{}, "Versatz"};
    juce::Slider offsetSlider_{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};

    JUCE_LEAK_DETECTOR(MetronomePanel)
};

} // namespace drumprog::app
