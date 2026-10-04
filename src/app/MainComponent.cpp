#include "app/MainComponent.h"

#include "app/Dialogs.h"
#include "ui/AudioStatus.h"

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 30;
constexpr int kLedSize = 10;
constexpr int kLedWidth = 90;

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
                             ui::KitPresenter& kitPresenter,
                             ui::KeymapPresenter& keymapPresenter,
                             ui::InputLedPresenter& inputLeds,
                             const juce::String& sampleWildcard)
    : testTone_(testTone), deviceManager_(deviceManager), inputLeds_(inputLeds),
      kitPanel_(kitPresenter, keymapPresenter, sampleWildcard)
{
    testToneButton_.setToggleState(testTone_.isEnabled(), juce::dontSendNotification);
    testToneButton_.onClick = [this] { testTone_.setEnabled(testToneButton_.getToggleState()); };
    settingsButton_.onClick = [this] { showAudioSettingsDialog(deviceManager_, this); };

    addAndMakeVisible(testToneButton_);
    addAndMakeVisible(settingsButton_);
    addAndMakeVisible(kitPanel_);
    addAndMakeVisible(status_);
    deviceManager_.addChangeListener(this);
    updateStatus();
    startTimerHz(kTimerHz);
    setSize(1000, 600);
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
    auto area = getLocalBounds().reduced(16);
    status_.setBounds(area.removeFromBottom(24));
    area.removeFromBottom(8);
    auto row = area.removeFromTop(32);
    testToneButton_.setBounds(row.removeFromLeft(200));
    settingsButton_.setBounds(row.removeFromRight(240));
    ledArea_ = row.removeFromRight(2 * kLedWidth + 16);
    area.removeFromTop(12);
    kitPanel_.setBounds(area);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    updateStatus();
}

void MainComponent::timerCallback()
{
    inputLeds_.tick();
    repaint(ledArea_);
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
