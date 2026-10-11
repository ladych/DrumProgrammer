#include "app/TransportBar.h"

#include "app/Theme.h"
#include "i18n/Translate.h"

#include <initializer_list>
#include <optional>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 30;
constexpr int kButtonWidth = 36;
constexpr int kGap = 4;
constexpr int kGroupGap = 16;
constexpr int kBlinkPeriod = 15; // frames of the 30 Hz timer: twice a second

/// Clicks must not take the keyboard focus, or the space bar and the drum keys would go to the widget.
void keepKeyboardFocusAway(std::initializer_list<juce::Component*> components)
{
    for (auto* component : components)
    {
        component->setWantsKeyboardFocus(false);
        component->setMouseClickGrabsKeyboardFocus(false);
    }
}

} // namespace

TransportBar::TransportBar(ui::TransportPresenter& transport, ui::TempoPresenter& tempo)
    : transport_(transport), tempo_(tempo)
{
    setUpTransport();
    setUpTempo();
    setUpMode();
    keepKeyboardFocusAway({&numeratorBox_, &denominatorBox_});
    refresh();
    refreshTempo();
    startTimerHz(kTimerHz);
}

TransportBar::~TransportBar()
{
    stopTimer();
}

void TransportBar::setUpTransport()
{
    rewindButton_.onClick = [this] { transport_.rewind(); };
    playButton_.onClick = [this] { transport_.play(); };
    stopButton_.onClick = [this] { transport_.stop(); };
    loopButton_.setClickingTogglesState(true);
    loopButton_.setToggleState(transport_.loop(), juce::dontSendNotification);
    loopButton_.onClick = [this] { transport_.setLoop(loopButton_.getToggleState()); };
    recButton_.onClick = [this] { transport_.toggleRecordArmed(); };
    // The current position stands out: green on a darker field with a frame (GUI draft, AP14).
    positionLabel_.setFont(juce::FontOptions{juce::Font::getDefaultMonospacedFontName(), 17.0F, 0});
    positionLabel_.setJustificationType(juce::Justification::centred);
    positionLabel_.setColour(juce::Label::textColourId, theme::kPosition);
    positionLabel_.setColour(juce::Label::backgroundColourId, theme::kPositionBackground);
    positionLabel_.setColour(juce::Label::outlineColourId, theme::kOutline);
    positionLabel_.setTooltip(i18n::trJuce("Position bar.beat.tick"));
    positionTitle_.setColour(juce::Label::textColourId, theme::kTextDim);
    positionTitle_.setJustificationType(juce::Justification::centredRight);
    for (auto* component : std::initializer_list<juce::Component*>{&rewindButton_,
                                                                   &playButton_,
                                                                   &stopButton_,
                                                                   &loopButton_,
                                                                   &recButton_,
                                                                   &positionTitle_,
                                                                   &positionLabel_})
        addAndMakeVisible(component);
}

void TransportBar::setUpTempo()
{
    bpmEditor_.setInputRestrictions(6, "0123456789.,");
    bpmEditor_.setJustification(juce::Justification::centred);
    bpmEditor_.setTooltip(i18n::trJuce("Tempo in BPM"));
    bpmEditor_.onReturnKey = [] { unfocusAllComponents(); };
    bpmEditor_.onEscapeKey = [this]
    {
        refreshTempo();
        unfocusAllComponents();
    };
    bpmEditor_.onFocusLost = [this] { commitBpm(); };
    for (int numerator = 1; numerator <= ui::TempoPresenter::kMaxNumerator; ++numerator)
        numeratorBox_.addItem(juce::String{numerator}, numerator);
    for (const int denominator : {4, 8, 16})
        denominatorBox_.addItem(juce::String{denominator}, denominator);
    numeratorBox_.setTooltip(i18n::trJuce("Beats per bar"));
    denominatorBox_.setTooltip(i18n::trJuce("Note value of a beat"));
    numeratorBox_.onChange = [this] { commitTimeSignature(); };
    denominatorBox_.onChange = [this] { commitTimeSignature(); };
    for (auto* label : {&bpmLabel_, &signatureLabel_, &modeLabel_})
    {
        label->setColour(juce::Label::textColourId, theme::kTextDim);
        label->setJustificationType(juce::Justification::centredRight);
    }
    for (auto* component : std::initializer_list<juce::Component*>{
             &bpmLabel_, &bpmEditor_, &signatureLabel_, &numeratorBox_, &denominatorBox_})
        addAndMakeVisible(component);
}

void TransportBar::setUpMode()
{
    patternModeButton_.onClick = [this] { transport_.setPlayMode(ui::PlayMode::pattern); };
    songModeButton_.onClick = [this] { transport_.setPlayMode(ui::PlayMode::song); };
    for (auto* component :
         std::initializer_list<juce::Component*>{&modeLabel_, &patternModeButton_, &songModeButton_})
        addAndMakeVisible(component);
}

void TransportBar::resized()
{
    auto row = getLocalBounds();
    for (auto* button : {&rewindButton_, &playButton_, &stopButton_, &loopButton_, &recButton_})
    {
        button->setBounds(row.removeFromLeft(kButtonWidth));
        row.removeFromLeft(kGap);
    }
    row.removeFromLeft(kGroupGap);
    positionTitle_.setBounds(row.removeFromLeft(62));
    positionLabel_.setBounds(row.removeFromLeft(120));
    row.removeFromLeft(kGroupGap);
    bpmLabel_.setBounds(row.removeFromLeft(40));
    bpmEditor_.setBounds(row.removeFromLeft(70).reduced(0, 1));
    row.removeFromLeft(kGap);
    signatureLabel_.setBounds(row.removeFromLeft(44));
    numeratorBox_.setBounds(row.removeFromLeft(56).reduced(0, 1));
    denominatorBox_.setBounds(row.removeFromLeft(56).reduced(0, 1));
    row.removeFromLeft(kGroupGap);
    modeLabel_.setBounds(row.removeFromLeft(54));
    row.removeFromLeft(kGap);
    patternModeButton_.setBounds(row.removeFromLeft(kButtonWidth));
    row.removeFromLeft(kGap);
    songModeButton_.setBounds(row.removeFromLeft(kButtonWidth));
}

void TransportBar::timerCallback()
{
    transport_.tick();
    refresh();
    if (!bpmEditor_.hasKeyboardFocus(false))
        refreshTempo();
}

void TransportBar::refresh()
{
    positionLabel_.setText(transport_.positionText(), juce::dontSendNotification);
    playButton_.setToggleState(transport_.isPlaying(), juce::dontSendNotification);
    loopButton_.setToggleState(transport_.loop(), juce::dontSendNotification);
    const bool song = transport_.playMode() == ui::PlayMode::song;
    patternModeButton_.setToggleState(!song, juce::dontSendNotification);
    songModeButton_.setToggleState(song, juce::dontSendNotification);
    // Rec: dark red while armed, bright red while recording, blinking during the count-in (F-TR-07).
    blink_ = (blink_ + 1) % kBlinkPeriod;
    std::optional<juce::Colour> recColour;
    const bool blinkOff = transport_.isCountingIn() && blink_ < kBlinkPeriod / 2;
    if (transport_.isRecording() && !blinkOff)
        recColour = theme::kRecord;
    else if (transport_.isRecordArmed())
        recColour = theme::kRecord.darker(1.2F);
    recButton_.setHighlight(recColour);
}

void TransportBar::refreshTempo()
{
    bpmEditor_.setText(tempo_.bpmText(), false);
    const auto signature = tempo_.timeSignature();
    numeratorBox_.setSelectedId(signature.numerator, juce::dontSendNotification);
    denominatorBox_.setSelectedId(signature.denominator, juce::dontSendNotification);
}

void TransportBar::commitBpm()
{
    tempo_.setBpmText(bpmEditor_.getText().toStdString());
    refreshTempo();
}

void TransportBar::commitTimeSignature()
{
    tempo_.setTimeSignature(numeratorBox_.getSelectedId(), denominatorBox_.getSelectedId());
    refreshTempo();
}

} // namespace drumprog::app
