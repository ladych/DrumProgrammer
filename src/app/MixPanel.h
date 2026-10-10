#pragma once

#include "ui/MixPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cstdint>

namespace drumprog::app
{

/// Humble object (E-03): the part "Mix (Referenz)" of the inspector (Pflichtenheft 6.1, area 10) with the
/// faders Backing, Drums and Master in dB (F-BT-06). Logic in ui::MixPresenter.
class MixPanel final : public juce::Component, private juce::Timer
{
public:
    static constexpr int kPreferredHeight = 120;

    explicit MixPanel(ui::MixPresenter& presenter);
    ~MixPanel() override;

    MixPanel(const MixPanel&) = delete;
    MixPanel& operator=(const MixPanel&) = delete;
    MixPanel(MixPanel&&) = delete;
    MixPanel& operator=(MixPanel&&) = delete;

    void resized() override;

private:
    struct Fader
    {
        juce::Label label;
        juce::Slider slider{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
    };

    void timerCallback() override;
    void refresh();

    ui::MixPresenter& presenter_;
    std::uint32_t seenChangeCount_ = 0;
    juce::Label title_{{}, "Mix (Referenz)"};
    std::array<Fader, ui::MixPresenter::kChannels.size()> faders_;
};

} // namespace drumprog::app
