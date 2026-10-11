#pragma once

#include "i18n/Translate.h"
#include "ui/PianoRollPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>

namespace drumprog::app
{

/// Humble object (E-03): the part "Ausgewählte Note" of the inspector (Pflichtenheft 6.1, area 10)
/// with instrument, position, length and velocity of the selected notes. Logic in
/// ui::PianoRollPresenter.
class NoteInspector final : public juce::Component, private juce::Timer
{
public:
    explicit NoteInspector(ui::PianoRollPresenter& presenter);
    ~NoteInspector() override;

    NoteInspector(const NoteInspector&) = delete;
    NoteInspector& operator=(const NoteInspector&) = delete;
    NoteInspector(NoteInspector&&) = delete;
    NoteInspector& operator=(NoteInspector&&) = delete;

    void resized() override;

private:
    void timerCallback() override;
    void refresh();

    ui::PianoRollPresenter& presenter_;
    std::uint32_t seenChangeCount_ = 0;
    juce::Label title_{{}, i18n::trJuce("Selected note")};
    juce::Label instrumentLabel_{{}, i18n::trJuce("Instrument")};
    juce::Label positionLabel_{{}, i18n::trJuce("Position")};
    juce::Label lengthLabel_{{}, i18n::trJuce("Length")};
    juce::Label velocityLabel_{{}, i18n::trJuce("Velocity")};
    juce::Label instrument_;
    juce::Label position_;
    juce::Label length_;
    juce::Slider velocity_{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
};

} // namespace drumprog::app
