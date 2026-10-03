#include "app/AppComposition.h"

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

std::filesystem::path deviceSettingsFile()
{
    const auto file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("DrumProgrammer")
                          .getChildFile("audio-device.xml");
    const std::string utf8 = file.getFullPathName().toStdString();
    // JUCE strings are UTF-8; std::filesystem only treats char8_t input as UTF-8 on every platform.
    return {std::u8string{utf8.begin(), utf8.end()}};
}

} // namespace

AppComposition::AppComposition()
    : deviceSettings_(fileSystem_, deviceSettingsFile()), testTone_(kTestToneFrequencyHz, kTestToneGain),
      kitBuilder_(sampleLoader_), kitPresenter_(engine::makeGmDefaultKit(), kitBuilder_, sampleEngine_),
      audioCallback_(testTone_, sampleEngine_)
{
    restoreDeviceSettings();
    updateSampleRate();
    deviceManager_.addAudioCallback(&audioCallback_);
    deviceManager_.addChangeListener(this);
}

AppComposition::~AppComposition()
{
    deviceManager_.removeChangeListener(this);
    deviceManager_.removeAudioCallback(&audioCallback_);
    deviceManager_.closeAudioDevice();
}

std::unique_ptr<juce::Component> AppComposition::createMainComponent()
{
    return std::make_unique<MainComponent>(
        testTone_, deviceManager_, kitPresenter_, sampleLoader_.wildcardPattern());
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
        kitPresenter_.setDeviceSampleRate(device->getCurrentSampleRate());
}

} // namespace drumprog::app
