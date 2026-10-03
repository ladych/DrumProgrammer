#pragma once

#include "engine/TestToneSource.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace drumprog::app
{

/// Humble object (E-03): main window content for milestone M0 with the
/// test tone switch and the audio/MIDI settings dialog (F-AO-01).
class MainComponent final : public juce::Component
{
public:
    MainComponent(engine::TestToneSource& testTone, juce::AudioDeviceManager& deviceManager);
    ~MainComponent() override = default;

    MainComponent(const MainComponent&) = delete;
    MainComponent& operator=(const MainComponent&) = delete;
    MainComponent(MainComponent&&) = delete;
    MainComponent& operator=(MainComponent&&) = delete;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void showSettingsDialog();

    engine::TestToneSource& testTone_;
    juce::AudioDeviceManager& deviceManager_;
    juce::ToggleButton testToneButton_{"Testton 440 Hz"};
    juce::TextButton settingsButton_{"Audio/MIDI-Einstellungen..."};

    JUCE_LEAK_DETECTOR(MainComponent)
};

} // namespace drumprog::app
