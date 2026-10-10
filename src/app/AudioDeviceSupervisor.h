#pragma once

#include "app/AudioCallback.h"
#include "audio/DeviceRecovery.h"
#include "audio/DriverChoice.h"
#include "io/SettingsStore.h"
#include "ui/AudioStatus.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <vector>

namespace drumprog::app
{

/// Humble object (E-03): opens the audio device and keeps it running (F-AO-03, F-AO-04, Q-09). The
/// decisions are in audio::driverTypesByPreference and audio::DeviceRecovery; this class only reads
/// and drives juce::AudioDeviceManager.
///
/// The device the user chose stays the saved one (JUCE's "explicit" settings) while a fallback plays,
/// so it comes back when it is available again and after the next start.
class AudioDeviceSupervisor final : private juce::Timer
{
public:
    AudioDeviceSupervisor(juce::AudioDeviceManager& deviceManager,
                          const io::SettingsStore& deviceSettings,
                          const AudioCallback& audioCallback,
                          int numOutputChannels);
    ~AudioDeviceSupervisor() override;

    AudioDeviceSupervisor(const AudioDeviceSupervisor&) = delete;
    AudioDeviceSupervisor& operator=(const AudioDeviceSupervisor&) = delete;
    AudioDeviceSupervisor(AudioDeviceSupervisor&&) = delete;
    AudioDeviceSupervisor& operator=(AudioDeviceSupervisor&&) = delete;

    /// Opens the saved device, or on the first start the best driver type (ASIO on Windows), and falls
    /// back to the next driver type if that fails. Starts watching the device.
    void open();
    /// Adds the stall, the fallback and the dropout count to the status line.
    void describe(ui::AudioDeviceInfo& info) const;

private:
    void timerCallback() override;
    void reopen();
    void openFallback(const juce::String& failedType);
    [[nodiscard]] std::unique_ptr<juce::XmlElement> chosenSetup() const;
    [[nodiscard]] bool isOnChosenDevice(const juce::XmlElement& setup) const;
    [[nodiscard]] std::vector<audio::DriverDevices> drivers();

    juce::AudioDeviceManager& deviceManager_;
    const io::SettingsStore& deviceSettings_;
    const AudioCallback& audioCallback_;
    int numOutputChannels_;
    audio::DeviceRecovery recovery_;
};

} // namespace drumprog::app
