#pragma once

#include "engine/TestToneSource.h"

#include <juce_audio_devices/juce_audio_devices.h>

namespace drumprog::app
{

/// Humble object (E-03): forwards the driver callback to the test tone.
class ToneAudioCallback final : public juce::AudioIODeviceCallback
{
public:
    explicit ToneAudioCallback(engine::TestToneSource& tone) : tone_(tone) {}

    void audioDeviceIOCallbackWithContext(const float* const* /*inputChannelData*/,
                                          int /*numInputChannels*/,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& /*context*/) override
    {
        tone_.render(outputChannelData, numOutputChannels, numSamples);
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        tone_.prepare(device->getCurrentSampleRate());
    }

    void audioDeviceStopped() override {}

private:
    engine::TestToneSource& tone_;
};

} // namespace drumprog::app
