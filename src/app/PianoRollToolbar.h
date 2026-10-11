#pragma once

#include "app/IconButton.h"
#include "i18n/Translate.h"
#include "ui/PianoRollPresenter.h"
#include "ui/TransportPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drumprog::app
{

/// Humble object (E-03): two button groups in the header of the piano roll, next to its zoom buttons
/// (AP14): the tools draw, select and erase (F-PR-02, F-PR-04) and the recording mode overdub or replace
/// (F-TR-04). Logic in ui::PianoRollPresenter and ui::TransportPresenter.
class PianoRollToolbar final : public juce::Component, private juce::Timer
{
public:
    static constexpr int kButtonSize = 26;
    static constexpr int kGroupGap = 12;
    static constexpr int kPreferredWidth = 5 * kButtonSize + 4 * 2 + kGroupGap;

    PianoRollToolbar(ui::PianoRollPresenter& presenter, ui::TransportPresenter& transport);
    ~PianoRollToolbar() override;

    PianoRollToolbar(const PianoRollToolbar&) = delete;
    PianoRollToolbar& operator=(const PianoRollToolbar&) = delete;
    PianoRollToolbar(PianoRollToolbar&&) = delete;
    PianoRollToolbar& operator=(PianoRollToolbar&&) = delete;

    void resized() override;

private:
    void timerCallback() override;
    void refresh();

    ui::PianoRollPresenter& presenter_;
    ui::TransportPresenter& transport_;
    IconButton drawButton_{Icon::draw, i18n::trJuce("Draw (Ctrl+1)")};
    IconButton selectButton_{Icon::select, i18n::trJuce("Select (Ctrl+2)")};
    IconButton eraseButton_{Icon::erase, i18n::trJuce("Erase (Ctrl+3)")};
    IconButton overdubButton_{Icon::overdub, i18n::trJuce("Recording overdub: adds to the notes")};
    IconButton replaceButton_{Icon::replace,
                              i18n::trJuce("Recording replace: clears the pattern at the start")};
};

/// Humble object (E-03): snap toggle, grid dropdown and the hint "Alt = frei" in the header of the piano
/// roll after its general information (F-PR-02, AP14). Logic in ui::PianoRollPresenter.
class SnapControls final : public juce::Component, private juce::Timer
{
public:
    static constexpr int kPreferredWidth = 220;

    explicit SnapControls(ui::PianoRollPresenter& presenter);
    ~SnapControls() override;

    SnapControls(const SnapControls&) = delete;
    SnapControls& operator=(const SnapControls&) = delete;
    SnapControls(SnapControls&&) = delete;
    SnapControls& operator=(SnapControls&&) = delete;

    void resized() override;

private:
    void timerCallback() override;
    void refresh();

    ui::PianoRollPresenter& presenter_;
    juce::ToggleButton snapButton_{"Snap"};
    juce::ComboBox gridBox_;
    juce::Label altHint_{{}, i18n::trJuce("Alt = free")};
};

} // namespace drumprog::app
