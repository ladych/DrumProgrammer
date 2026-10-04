#pragma once

#include "app/KitPanel.h"
#include "app/TransportBar.h"
#include "engine/TestToneSource.h"
#include "ui/InputLedPresenter.h"
#include "ui/KeymapPresenter.h"
#include "ui/KitPresenter.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace drumprog::app
{

/// Humble object (E-03): main window content with the transport bar (F-TR-01 to 09), the test tone
/// switch, the input LEDs "MIDI In" and "Tastatur" (F-IN-05), the kit panel and a status line with
/// device and latency (Q-02).
class MainComponent final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    MainComponent(engine::TestToneSource& testTone,
                  juce::AudioDeviceManager& deviceManager,
                  ui::TransportPresenter& transport,
                  ui::TempoPresenter& tempo,
                  ui::KitPresenter& kitPresenter,
                  ui::KeymapPresenter& keymapPresenter,
                  ui::InputLedPresenter& inputLeds,
                  const juce::String& sampleWildcard);
    ~MainComponent() override;

    MainComponent(const MainComponent&) = delete;
    MainComponent& operator=(const MainComponent&) = delete;
    MainComponent(MainComponent&&) = delete;
    MainComponent& operator=(MainComponent&&) = delete;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void timerCallback() override;
    void updateStatus();
    void paintLed(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& label, bool on) const;

    engine::TestToneSource& testTone_;
    juce::AudioDeviceManager& deviceManager_;
    ui::InputLedPresenter& inputLeds_;
    juce::ToggleButton testToneButton_{"Testton 440 Hz"};
    juce::TextButton settingsButton_{"Audio/MIDI-Einstellungen..."};
    juce::Rectangle<int> ledArea_;
    TransportBar transportBar_;
    KitPanel kitPanel_;
    juce::Label status_;

    JUCE_LEAK_DETECTOR(MainComponent)
};

} // namespace drumprog::app
