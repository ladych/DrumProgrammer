#include "app/MainComponent.h"

#include <memory>

namespace drumprog::app
{

MainComponent::MainComponent(engine::TestToneSource& testTone,
                             juce::AudioDeviceManager& deviceManager,
                             ui::KitPresenter& kitPresenter,
                             const juce::String& sampleWildcard)
    : testTone_(testTone), deviceManager_(deviceManager), kitPanel_(kitPresenter, sampleWildcard)
{
    testToneButton_.setToggleState(testTone_.isEnabled(), juce::dontSendNotification);
    testToneButton_.onClick = [this] { testTone_.setEnabled(testToneButton_.getToggleState()); };
    settingsButton_.onClick = [this] { showSettingsDialog(); };

    addAndMakeVisible(testToneButton_);
    addAndMakeVisible(settingsButton_);
    addAndMakeVisible(kitPanel_);
    setSize(900, 560);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(16);
    auto row = area.removeFromTop(32);
    testToneButton_.setBounds(row.removeFromLeft(200));
    settingsButton_.setBounds(row.removeFromRight(240));
    area.removeFromTop(12);
    kitPanel_.setBounds(area);
}

void MainComponent::showSettingsDialog()
{
    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager_,
                                                                         0,
                                                                         0, // audio inputs
                                                                         0,
                                                                         2,     // audio outputs
                                                                         true,  // MIDI inputs
                                                                         false, // MIDI output
                                                                         true,  // stereo pairs
                                                                         false);
    selector->setSize(560, 440);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector.release());
    options.dialogTitle = "Audio/MIDI-Einstellungen";
    options.componentToCentreAround = this;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

} // namespace drumprog::app
