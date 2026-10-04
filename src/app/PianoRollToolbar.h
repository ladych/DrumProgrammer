#pragma once

#include "ui/PianoRollPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drumprog::app
{

/// Humble object (E-03): snap and tools of the piano roll (Pflichtenheft 6.1, area 2): snap toggle,
/// grid dropdown, the hint "Alt = frei" and the tools draw, select and erase (F-PR-02, F-PR-04).
class PianoRollToolbar final : public juce::Component, private juce::Timer
{
public:
    explicit PianoRollToolbar(ui::PianoRollPresenter& presenter);
    ~PianoRollToolbar() override;

    PianoRollToolbar(const PianoRollToolbar&) = delete;
    PianoRollToolbar& operator=(const PianoRollToolbar&) = delete;
    PianoRollToolbar(PianoRollToolbar&&) = delete;
    PianoRollToolbar& operator=(PianoRollToolbar&&) = delete;

    void resized() override;

private:
    void timerCallback() override;
    void setUpGrid();
    void setUpTools();
    void refresh();

    ui::PianoRollPresenter& presenter_;
    juce::ToggleButton snapButton_{"Snap"};
    juce::ComboBox gridBox_;
    juce::Label altHint_{{}, "Alt = frei"};
    juce::TextButton drawButton_{"Zeichnen"};
    juce::TextButton selectButton_{"Auswahl"};
    juce::TextButton eraseButton_{juce::String::fromUTF8("L\xc3\xb6schen")};
};

} // namespace drumprog::app
