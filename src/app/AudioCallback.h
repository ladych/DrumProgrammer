#pragma once

#include "engine/Clock.h"
#include "engine/PlaybackRenderer.h"
#include "engine/TestToneSource.h"
#include "model/SnapshotPublisher.h"

#include <juce_audio_devices/juce_audio_devices.h>

namespace drumprog::app
{

/// Humble object (E-03): forwards the driver callback to the test tone and the playback renderer,
/// with the current project snapshot and the time the block started.
class AudioCallback final : public juce::AudioIODeviceCallback
{
public:
    AudioCallback(engine::TestToneSource& tone,
                  engine::PlaybackRenderer& renderer,
                  model::ProjectSnapshotExchange& snapshots)
        : tone_(tone), renderer_(renderer), snapshots_(snapshots)
    {
    }

    void audioDeviceIOCallbackWithContext(const float* const* /*inputChannelData*/,
                                          int /*numInputChannels*/,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& /*context*/) override
    {
        const double blockTime = engine::steadyClockSeconds();
        // The tone overwrites the outputs (silence while disabled), the renderer adds to them.
        tone_.render(outputChannelData, numOutputChannels, numSamples);
        renderer_.render(snapshots_.acquire(), outputChannelData, numOutputChannels, numSamples, blockTime);
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        const double sampleRate = device->getCurrentSampleRate();
        tone_.prepare(sampleRate);
        renderer_.prepare(sampleRate);
    }

    void audioDeviceStopped() override {}

private:
    engine::TestToneSource& tone_;
    engine::PlaybackRenderer& renderer_;
    model::ProjectSnapshotExchange& snapshots_;
};

} // namespace drumprog::app
