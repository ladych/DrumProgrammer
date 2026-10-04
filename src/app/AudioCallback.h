#pragma once

#include "engine/SampleEngine.h"
#include "engine/TestToneSource.h"

#include <juce_audio_devices/juce_audio_devices.h>

namespace drumprog::app
{

/// Humble object (E-03): forwards the driver callback to the test tone and the sample engine.
class AudioCallback final : public juce::AudioIODeviceCallback
{
public:
    AudioCallback(engine::TestToneSource& tone, engine::SampleEngine& engine) : tone_(tone), engine_(engine)
    {
    }

    void audioDeviceIOCallbackWithContext(const float* const* /*inputChannelData*/,
                                          int /*numInputChannels*/,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& /*context*/) override
    {
        // The tone overwrites the outputs (silence while disabled), the engine adds to them.
        tone_.render(outputChannelData, numOutputChannels, numSamples);
        engine_.render(outputChannelData, numOutputChannels, numSamples);
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        const double sampleRate = device->getCurrentSampleRate();
        tone_.prepare(sampleRate);
        engine_.prepare(sampleRate);
    }

    void audioDeviceStopped() override {}

private:
    engine::TestToneSource& tone_;
    engine::SampleEngine& engine_;
};

} // namespace drumprog::app
