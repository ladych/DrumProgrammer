#include "app/MixPanel.h"

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 10;
constexpr int kLabelWidth = 64;
constexpr int kRowHeight = 26;

} // namespace

MixPanel::MixPanel(ui::MixPresenter& presenter) : presenter_(presenter)
{
    title_.setFont(juce::FontOptions(15.0F, juce::Font::bold));
    addAndMakeVisible(title_);
    for (std::size_t index = 0; index < faders_.size(); ++index)
    {
        auto& fader = faders_.at(index);
        const auto channel = ui::MixPresenter::kChannels.at(index);
        fader.label.setText(juce::String::fromUTF8(ui::MixPresenter::label(channel).c_str()),
                            juce::dontSendNotification);
        fader.slider.setRange(ui::MixPresenter::kMinGainDb, ui::MixPresenter::kMaxGainDb, 0.1);
        fader.slider.setTextValueSuffix(" dB");
        fader.slider.setDoubleClickReturnValue(true, 0.0);
        fader.slider.setWantsKeyboardFocus(false);
        fader.slider.onValueChange = [this, channel, &fader]
        { presenter_.setGainDb(channel, fader.slider.getValue()); };
        addAndMakeVisible(fader.label);
        addAndMakeVisible(fader.slider);
    }
    refresh();
    startTimerHz(kTimerHz);
}

MixPanel::~MixPanel()
{
    stopTimer();
}

void MixPanel::resized()
{
    auto area = getLocalBounds().reduced(4);
    title_.setBounds(area.removeFromTop(22));
    for (auto& fader : faders_)
    {
        auto row = area.removeFromTop(kRowHeight);
        fader.label.setBounds(row.removeFromLeft(kLabelWidth));
        fader.slider.setBounds(row);
    }
}

void MixPanel::timerCallback()
{
    if (presenter_.changeCount() != seenChangeCount_)
        refresh();
}

void MixPanel::refresh()
{
    seenChangeCount_ = presenter_.changeCount();
    for (std::size_t index = 0; index < faders_.size(); ++index)
        faders_.at(index).slider.setValue(presenter_.gainDb(ui::MixPresenter::kChannels.at(index)),
                                          juce::dontSendNotification);
}

} // namespace drumprog::app
