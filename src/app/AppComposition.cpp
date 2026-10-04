#include "app/AppComposition.h"

#include "app/Dialogs.h"
#include "app/MainComponent.h"

#include <filesystem>
#include <string>

namespace drumprog::app
{
namespace
{

constexpr double kTestToneFrequencyHz = 440.0;
constexpr float kTestToneGain = 0.2F;
constexpr int kNumOutputChannels = 2;

std::filesystem::path settingsFile(const char* name)
{
    const auto file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("DrumProgrammer")
                          .getChildFile(name);
    const std::string utf8 = file.getFullPathName().toStdString();
    // JUCE strings are UTF-8; std::filesystem only treats char8_t input as UTF-8 on every platform.
    return {std::u8string{utf8.begin(), utf8.end()}};
}

} // namespace

AppComposition::AppComposition()
    : deviceSettings_(fileSystem_, settingsFile("audio-device.xml")),
      keymapSettings_(fileSystem_, settingsFile("keymap.txt")),
      testTone_(kTestToneFrequencyHz, kTestToneGain), kitBuilder_(sampleLoader_),
      kitPublisher_(kitBuilder_, sampleEngine_), audioCallback_(testTone_, sampleEngine_),
      projectFactory_(idGenerator_), project_(projectFactory_.createDefault()),
      snapshotPublisher_(project_, snapshots_, kitPublisher_),
      kitPresenter_(project_, undoManager_, kitPublisher_, sampleEngine_),
      keymap_(input::Keymap::fromText(keymapSettings_.load())),
      keymapPresenter_(keymap_, keymapSettings_, keyNames_, kitPresenter_), guiNoteSink_(sampleEngine_),
      keyboardInput_(keymap_, guiNoteSink_, inputActivity_), keyRouter_(keymapPresenter_, keyboardInput_),
      keyboardListener_(keyRouter_), midiNoteSink_(sampleEngine_),
      midiInputHandler_(midiNoteSink_, inputActivity_), midiInputCallback_(midiInputHandler_),
      inputLeds_(inputActivity_), projectRepository_(fileSystem_),
      documentController_(project_, undoManager_, projectRepository_, projectFactory_, documentView_),
      mainMenu_(documentController_,
                [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); },
                {.showSettings = [this] { showAudioSettingsDialog(deviceManager_, dialogParent_); },
                 .showKeymap = [this] { showKeymapDialog(keymapPresenter_, dialogParent_); }})
{
    restoreDeviceSettings();
    updateSampleRate();
    deviceManager_.addAudioCallback(&audioCallback_);
    // Empty identifier: messages of every MIDI input enabled in the settings (F-IN-03).
    deviceManager_.addMidiInputDeviceCallback({}, &midiInputCallback_);
    deviceManager_.addChangeListener(this);
}

AppComposition::~AppComposition()
{
    deviceManager_.removeChangeListener(this);
    deviceManager_.removeMidiInputDeviceCallback({}, &midiInputCallback_);
    deviceManager_.removeAudioCallback(&audioCallback_);
    deviceManager_.closeAudioDevice();
}

std::unique_ptr<juce::Component> AppComposition::createMainComponent()
{
    return std::make_unique<MainComponent>(testTone_,
                                           deviceManager_,
                                           kitPresenter_,
                                           keymapPresenter_,
                                           inputLeds_,
                                           sampleLoader_.wildcardPattern());
}

void AppComposition::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    saveDeviceSettings();
    updateSampleRate();
}

void AppComposition::restoreDeviceSettings()
{
    const auto savedState = deviceSettings_.load();
    const auto xml = savedState ? juce::parseXML(juce::String::fromUTF8(savedState->c_str())) : nullptr;
    const auto error = deviceManager_.initialise(0, kNumOutputChannels, xml.get(), true);
    if (error.isNotEmpty())
        juce::Logger::writeToLog("Audio device could not be opened: " + error);
}

void AppComposition::saveDeviceSettings()
{
    if (const auto xml = deviceManager_.createStateXml())
        deviceSettings_.save(xml->toString().toStdString());
}

void AppComposition::updateSampleRate()
{
    // Samples are resampled to the device rate when the kit is built (F-SE-02).
    if (auto* device = deviceManager_.getCurrentAudioDevice())
        kitPublisher_.setDeviceSampleRate(device->getCurrentSampleRate());
}

} // namespace drumprog::app
