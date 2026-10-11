#pragma once

#include "i18n/Translate.h"
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
    juce::Label title_{{}, i18n::trJuce("Metronome and recording")};
    juce::ToggleButton metronomeButton_{i18n::trJuce("Metronome")};
    juce::ToggleButton metronomeRecordButton_{i18n::trJuce("when recording")};
    juce::Label levelLabel_{{}, i18n::trJuce("Level")};
    juce::Slider levelSlider_{juce::Slider::LinearHorizontal, juce::Slider::NoTextBox};
    juce::Label countInLabel_{{}, i18n::trJuce("Count-in")};
    juce::ComboBox countInBox_;
    juce::Label offsetLabel_{{}, i18n::trJuce("Offset")};
    juce::Slider offsetSlider_{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};

    JUCE_LEAK_DETECTOR(MetronomePanel)
};

} // namespace drumprog::app
