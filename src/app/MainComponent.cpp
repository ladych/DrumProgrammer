#include "app/MainComponent.h"

#include "app/Dialogs.h"
#include "ui/AudioStatus.h"

#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 30;
constexpr int kLedSize = 10;
constexpr int kLedWidth = 90;
constexpr int kSideWidth = 260;
constexpr int kKitPanelHeight = 290;

ui::AudioDeviceInfo deviceInfo(juce::AudioDeviceManager& deviceManager)
{
    ui::AudioDeviceInfo info;
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        info.open = true;
        info.driver = device->getTypeName().toStdString();
        info.device = device->getName().toStdString();
        info.sampleRate = device->getCurrentSampleRate();
        info.bufferSize = device->getCurrentBufferSizeSamples();
        info.outputLatencySamples = device->getOutputLatencyInSamples();
    }
    for (const auto& input : juce::MidiInput::getAvailableDevices())
        if (deviceManager.isMidiInputDeviceEnabled(input.identifier))
            info.midiInputs.push_back(input.name.toStdString());
    return info;
}

} // namespace

MainComponent::MainComponent(engine::TestToneSource& testTone,
                             juce::AudioDeviceManager& deviceManager,
                             ui::TransportPresenter& transport,
                             ui::TempoPresenter& tempo,
                             ui::KitPresenter& kitPresenter,
                             ui::PatternListPresenter& patterns,
                             PatternDialogs patternDialogs,
                             ui::SongTimelinePresenter& songTimeline,
                             ui::BackingTrackPresenter& backingTrack,
                             juce::AudioFormatManager& backingFormats,
                             std::function<void()> loadBackingTrack,
                             ui::MixPresenter& mix,
                             ui::PianoRollPresenter& pianoRoll,
                             ui::KeymapPresenter& keymapPresenter,
                             ui::InputLedPresenter& inputLeds,
                             const juce::String& sampleWildcard)
    : testTone_(testTone), deviceManager_(deviceManager), inputLeds_(inputLeds), pianoRoll_(pianoRoll),
      transportBar_(transport, tempo), pianoRollToolbar_(pianoRoll),
      patternList_(patterns, std::move(patternDialogs)),
      songTimeline_(songTimeline, transport, backingTrack, backingFormats, std::move(loadBackingTrack)),
      pianoRollView_(pianoRoll, transport), noteInspector_(pianoRoll), mixPanel_(mix),
      kitPanel_(kitPresenter, keymapPresenter, sampleWildcard)
{
    testToneButton_.setToggleState(testTone_.isEnabled(), juce::dontSendNotification);
    testToneButton_.onClick = [this] { testTone_.setEnabled(testToneButton_.getToggleState()); };
    settingsButton_.onClick = [this] { showAudioSettingsDialog(deviceManager_, this); };

    testToneButton_.setWantsKeyboardFocus(false);
    settingsButton_.setWantsKeyboardFocus(false);
    editStatus_.setJustificationType(juce::Justification::centredRight);
    for (auto* component : std::initializer_list<juce::Component*>{&transportBar_,
                                                                   &testToneButton_,
                                                                   &settingsButton_,
                                                                   &pianoRollToolbar_,
                                                                   &patternList_,
                                                                   &songTimeline_,
                                                                   &pianoRollView_,
                                                                   &noteInspector_,
                                                                   &mixPanel_,
                                                                   &kitPanel_,
                                                                   &status_,
                                                                   &editStatus_})
        addAndMakeVisible(component);
    deviceManager_.addChangeListener(this);
    updateStatus();
    startTimerHz(kTimerHz);
    setSize(1440, 960);
}

MainComponent::~MainComponent()
{
    stopTimer();
    deviceManager_.removeChangeListener(this);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    auto leds = ledArea_;
    paintLed(g, leds.removeFromLeft(kLedWidth), "MIDI In", inputLeds_.isMidiLedOn());
    paintLed(g, leds.removeFromLeft(kLedWidth), "Tastatur", inputLeds_.isKeyboardLedOn());
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    auto statusRow = area.removeFromBottom(24);
    editStatus_.setBounds(statusRow.removeFromRight(320));
    status_.setBounds(statusRow);
    area.removeFromBottom(6);
    transportBar_.setBounds(area.removeFromTop(TransportBar::kPreferredHeight));
    area.removeFromTop(6);
    auto row = area.removeFromTop(30);
    testToneButton_.setBounds(row.removeFromLeft(160));
    settingsButton_.setBounds(row.removeFromRight(220));
    ledArea_ = row.removeFromRight(2 * kLedWidth + 16);
    pianoRollToolbar_.setBounds(row.withTrimmedLeft(16));
    area.removeFromTop(8);
    kitPanel_.setBounds(area.removeFromBottom(kKitPanelHeight));
    area.removeFromBottom(8);
    patternList_.setBounds(area.removeFromLeft(kSideWidth));
    area.removeFromLeft(8);
    auto inspector = area.removeFromRight(kSideWidth).withTrimmedLeft(8);
    mixPanel_.setBounds(inspector.removeFromBottom(MixPanel::kPreferredHeight));
    noteInspector_.setBounds(inspector);
    songTimeline_.setBounds(area.removeFromTop(SongTimelineView::kPreferredHeight));
    area.removeFromTop(8);
    pianoRollView_.setBounds(area);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    updateStatus();
}

void MainComponent::timerCallback()
{
    inputLeds_.tick();
    repaint(ledArea_);
    editStatus_.setText(juce::String::fromUTF8(pianoRoll_.statusText().c_str()), juce::dontSendNotification);
}

void MainComponent::updateStatus()
{
    status_.setText(juce::String::fromUTF8(ui::audioStatusText(deviceInfo(deviceManager_)).c_str()),
                    juce::dontSendNotification);
}

void MainComponent::paintLed(juce::Graphics& g,
                             juce::Rectangle<int> area,
                             const juce::String& label,
                             bool on) const
{
    const auto led = area.removeFromLeft(kLedSize + 6).withSizeKeepingCentre(kLedSize, kLedSize).toFloat();
    g.setColour(on ? juce::Colours::limegreen : juce::Colours::darkgrey);
    g.fillEllipse(led);
    g.setColour(getLookAndFeel().findColour(juce::Label::textColourId));
    g.drawText(label, area, juce::Justification::centredLeft);
}

} // namespace drumprog::app
