#include "app/MetronomePanel.h"

#include "app/Theme.h"
#include "i18n/Translate.h"

#include <array>
#include <initializer_list>
#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kTitleHeight = 26;
constexpr int kRowHeight = 26;
constexpr int kLabelWidth = 80;

} // namespace

MetronomePanel::MetronomePanel(ui::TransportPresenter& transport) : transport_(transport)
{
    title_.setFont(juce::FontOptions(15.0F, juce::Font::bold));
    setUpMetronome();
    setUpRecording();
    for (auto* component : std::initializer_list<juce::Component*>{&title_,
                                                                   &metronomeButton_,
                                                                   &metronomeRecordButton_,
                                                                   &levelLabel_,
                                                                   &levelSlider_,
                                                                   &countInLabel_,
                                                                   &countInBox_,
                                                                   &offsetLabel_,
                                                                   &offsetSlider_})
    {
        // Clicks must not take the keyboard focus, or the space bar and the drum keys would go to the widget.
        component->setWantsKeyboardFocus(false);
        component->setMouseClickGrabsKeyboardFocus(false);
        addAndMakeVisible(component);
    }
}

void MetronomePanel::setUpMetronome()
{
    metronomeButton_.setTooltip(i18n::trJuce("Metronome during playback"));
    metronomeButton_.setToggleState(transport_.metronomeOnPlayback(), juce::dontSendNotification);
    metronomeButton_.onClick = [this]
    { transport_.setMetronomeOnPlayback(metronomeButton_.getToggleState()); };
    metronomeRecordButton_.setTooltip(i18n::trJuce("Metronome during recording"));
    metronomeRecordButton_.setToggleState(transport_.metronomeOnRecord(), juce::dontSendNotification);
    metronomeRecordButton_.onClick = [this]
    { transport_.setMetronomeOnRecord(metronomeRecordButton_.getToggleState()); };
    levelSlider_.setRange(0.0, 1.0);
    levelSlider_.setValue(transport_.metronomeLevel(), juce::dontSendNotification);
    levelSlider_.setTooltip(i18n::trJuce("Metronome level"));
    levelSlider_.onValueChange = [this]
    { transport_.setMetronomeLevel(static_cast<float>(levelSlider_.getValue())); };
}

void MetronomePanel::setUpRecording()
{
    countInBox_.addItemList({i18n::trJuce("off"), i18n::trJuce("%1 bar", "1"), i18n::trJuce("%1 bars", "2")},
                            1);
    countInBox_.setTooltip(i18n::trJuce("Count-in before recording"));
    countInBox_.setSelectedItemIndex(transport_.countInBars(), juce::dontSendNotification);
    countInBox_.onChange = [this] { transport_.setCountInBars(countInBox_.getSelectedItemIndex()); };
    const double maxOffset = ui::TransportPresenter::kMaxRecordOffsetMs;
    offsetSlider_.setRange(-maxOffset, maxOffset, 0.5);
    offsetSlider_.setTextValueSuffix(" ms");
    offsetSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
    offsetSlider_.setValue(transport_.recordOffsetMs(), juce::dontSendNotification);
    offsetSlider_.setTooltip(i18n::trJuce("Moves recorded notes; positive = later"));
    offsetSlider_.onValueChange = [this] { transport_.setRecordOffsetMs(offsetSlider_.getValue()); };
}

void MetronomePanel::resized()
{
    auto area = getLocalBounds();
    title_.setBounds(area.removeFromTop(kTitleHeight));
    metronomeButton_.setBounds(area.removeFromTop(kRowHeight));
    metronomeRecordButton_.setBounds(area.removeFromTop(kRowHeight));
    const std::array<std::pair<juce::Label*, juce::Component*>, 3> rows{
        {{&levelLabel_, &levelSlider_}, {&countInLabel_, &countInBox_}, {&offsetLabel_, &offsetSlider_}}};
    for (const auto& [label, value] : rows)
    {
        auto row = area.removeFromTop(kRowHeight);
        label->setBounds(row.removeFromLeft(kLabelWidth));
        value->setBounds(row.reduced(0, 2));
    }
}

} // namespace drumprog::app
