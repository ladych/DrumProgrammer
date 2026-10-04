#include "app/TransportBar.h"

#include <initializer_list>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 30;
constexpr int kRowHeight = 30;
constexpr int kGap = 6;
constexpr int kBlinkPeriod = 15; // frames of the 30 Hz timer: twice a second

const juce::Colour kRecordRed{0xffd03030};
const juce::Colour kActiveOrange{0xfff0a020};

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
    setUpMetronome();
    setUpRecording();
    keepKeyboardFocusAway({&rewindButton_,
                           &playButton_,
                           &stopButton_,
                           &loopButton_,
                           &recButton_,
                           &numeratorBox_,
                           &denominatorBox_,
                           &patternModeButton_,
                           &songModeButton_,
                           &metronomeButton_,
                           &metronomeRecordButton_,
                           &metronomeLevel_,
                           &countInBox_,
                           &recordModeBox_,
                           &offsetSlider_});
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
    rewindButton_.setTooltip(juce::String::fromUTF8("Zurück zum Anfang"));
    rewindButton_.onClick = [this] { transport_.rewind(); };
    playButton_.setTooltip("Wiedergabe (Leertaste)");
    playButton_.onClick = [this] { transport_.play(); };
    stopButton_.setTooltip("Stopp (Leertaste)");
    stopButton_.onClick = [this] { transport_.stop(); };
    loopButton_.setClickingTogglesState(true);
    loopButton_.setToggleState(transport_.loop(), juce::dontSendNotification);
    loopButton_.onClick = [this] { transport_.setLoop(loopButton_.getToggleState()); };
    recButton_.setTooltip("Aufnahme scharf schalten (Strg+R)");
    recButton_.onClick = [this] { transport_.toggleRecordArmed(); };
    positionLabel_.setFont(juce::FontOptions{juce::Font::getDefaultMonospacedFontName(), 18.0F, 0});
    positionLabel_.setJustificationType(juce::Justification::centred);
    for (auto* component : std::initializer_list<juce::Component*>{
             &rewindButton_, &playButton_, &stopButton_, &loopButton_, &recButton_, &positionLabel_})
        addAndMakeVisible(component);
}

void TransportBar::setUpTempo()
{
    bpmEditor_.setInputRestrictions(6, "0123456789.,");
    bpmEditor_.setJustification(juce::Justification::centred);
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
    numeratorBox_.onChange = [this] { commitTimeSignature(); };
    denominatorBox_.onChange = [this] { commitTimeSignature(); };
    for (auto* component : std::initializer_list<juce::Component*>{
             &bpmLabel_, &bpmEditor_, &signatureLabel_, &numeratorBox_, &denominatorBox_})
        addAndMakeVisible(component);
}

void TransportBar::setUpMode()
{
    patternModeButton_.setTooltip("Spielt das aktive Pattern");
    patternModeButton_.onClick = [this] { transport_.setPlayMode(ui::PlayMode::pattern); };
    songModeButton_.setTooltip("Spielt die Song-Timeline");
    songModeButton_.onClick = [this] { transport_.setPlayMode(ui::PlayMode::song); };
    for (auto* component :
         std::initializer_list<juce::Component*>{&modeLabel_, &patternModeButton_, &songModeButton_})
        addAndMakeVisible(component);
}

void TransportBar::setUpMetronome()
{
    metronomeButton_.setToggleState(transport_.metronomeOnPlayback(), juce::dontSendNotification);
    metronomeButton_.onClick = [this]
    { transport_.setMetronomeOnPlayback(metronomeButton_.getToggleState()); };
    metronomeRecordButton_.setToggleState(transport_.metronomeOnRecord(), juce::dontSendNotification);
    metronomeRecordButton_.onClick = [this]
    { transport_.setMetronomeOnRecord(metronomeRecordButton_.getToggleState()); };
    metronomeLevel_.setRange(0.0, 1.0);
    metronomeLevel_.setValue(transport_.metronomeLevel(), juce::dontSendNotification);
    metronomeLevel_.setTooltip("Metronom-Pegel");
    metronomeLevel_.onValueChange = [this]
    { transport_.setMetronomeLevel(static_cast<float>(metronomeLevel_.getValue())); };
    for (auto* component : std::initializer_list<juce::Component*>{
             &metronomeButton_, &metronomeRecordButton_, &metronomeLevel_})
        addAndMakeVisible(component);
}

void TransportBar::setUpRecording()
{
    countInBox_.addItemList({juce::String::fromUTF8("Vorzähler aus"),
                             juce::String::fromUTF8("Vorzähler 1 Takt"),
                             juce::String::fromUTF8("Vorzähler 2 Takte")},
                            1);
    countInBox_.setSelectedItemIndex(transport_.countInBars(), juce::dontSendNotification);
    countInBox_.onChange = [this] { transport_.setCountInBars(countInBox_.getSelectedItemIndex()); };
    recordModeBox_.addItemList({"Overdub", "Ersetzen"}, 1);
    recordModeBox_.setSelectedItemIndex(0, juce::dontSendNotification);
    recordModeBox_.onChange = [this]
    {
        transport_.setRecordMode(recordModeBox_.getSelectedItemIndex() == 0 ? model::RecordMode::overdub
                                                                            : model::RecordMode::replace);
    };
    const double maxOffset = ui::TransportPresenter::kMaxRecordOffsetMs;
    offsetSlider_.setRange(-maxOffset, maxOffset, 0.5);
    offsetSlider_.setValue(transport_.recordOffsetMs(), juce::dontSendNotification);
    offsetSlider_.setTooltip(juce::String::fromUTF8("Verschiebt aufgenommene Noten; positiv = später"));
    offsetSlider_.onValueChange = [this] { transport_.setRecordOffsetMs(offsetSlider_.getValue()); };
    for (auto* component : std::initializer_list<juce::Component*>{
             &countInBox_, &recordModeBox_, &offsetLabel_, &offsetSlider_})
        addAndMakeVisible(component);
}

void TransportBar::resized()
{
    auto area = getLocalBounds();
    auto row = area.removeFromTop(kRowHeight);
    for (auto* button : {&rewindButton_, &playButton_, &stopButton_, &loopButton_, &recButton_})
    {
        button->setBounds(row.removeFromLeft(56));
        row.removeFromLeft(kGap);
    }
    positionLabel_.setBounds(row.removeFromLeft(130));
    row.removeFromLeft(kGap);
    bpmLabel_.setBounds(row.removeFromLeft(40));
    bpmEditor_.setBounds(row.removeFromLeft(70).reduced(0, 2));
    row.removeFromLeft(kGap);
    signatureLabel_.setBounds(row.removeFromLeft(44));
    numeratorBox_.setBounds(row.removeFromLeft(56).reduced(0, 2));
    denominatorBox_.setBounds(row.removeFromLeft(56).reduced(0, 2));
    row.removeFromLeft(2 * kGap);
    modeLabel_.setBounds(row.removeFromLeft(54));
    patternModeButton_.setBounds(row.removeFromLeft(70));
    songModeButton_.setBounds(row.removeFromLeft(60));

    area.removeFromTop(kGap);
    row = area.removeFromTop(kRowHeight);
    metronomeButton_.setBounds(row.removeFromLeft(100));
    metronomeRecordButton_.setBounds(row.removeFromLeft(120));
    metronomeLevel_.setBounds(row.removeFromLeft(100));
    row.removeFromLeft(kGap);
    countInBox_.setBounds(row.removeFromLeft(150).reduced(0, 2));
    row.removeFromLeft(kGap);
    recordModeBox_.setBounds(row.removeFromLeft(110).reduced(0, 2));
    row.removeFromLeft(kGap);
    offsetLabel_.setBounds(row.removeFromLeft(90));
    offsetSlider_.setBounds(row.removeFromLeft(200));
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
    const bool playing = transport_.isPlaying();
    playButton_.setColour(juce::TextButton::buttonColourId,
                          playing ? kActiveOrange
                                  : getLookAndFeel().findColour(juce::TextButton::buttonColourId));
    loopButton_.setToggleState(transport_.loop(), juce::dontSendNotification);
    const auto normal = getLookAndFeel().findColour(juce::TextButton::buttonColourId);
    const bool song = transport_.playMode() == ui::PlayMode::song;
    patternModeButton_.setColour(juce::TextButton::buttonColourId, song ? normal : kActiveOrange);
    songModeButton_.setColour(juce::TextButton::buttonColourId, song ? kActiveOrange : normal);
    // Rec: dark red while armed, bright red while recording, blinking during the count-in (F-TR-07).
    blink_ = (blink_ + 1) % kBlinkPeriod;
    juce::Colour recColour = getLookAndFeel().findColour(juce::TextButton::buttonColourId);
    const bool blinkOff = transport_.isCountingIn() && blink_ < kBlinkPeriod / 2;
    if (transport_.isRecording() && !blinkOff)
        recColour = kRecordRed;
    else if (transport_.isRecordArmed())
        recColour = kRecordRed.darker(1.2F);
    recButton_.setColour(juce::TextButton::buttonColourId, recColour);
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
