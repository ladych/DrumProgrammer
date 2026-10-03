#pragma once

#include "app/AudioCallback.h"
#include "app/JuceSampleLoader.h"
#include "engine/KitBuilder.h"
#include "engine/SampleEngine.h"
#include "engine/TestToneSource.h"
#include "io/DeviceSettingsStore.h"
#include "io/StdFileSystem.h"
#include "ui/KitPresenter.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>

namespace drumprog::app
{

/// Composition root (E-05): the only place where the object graph is created
/// and wired. Members are declared in dependency order, so they are destroyed
/// in reverse.
class AppComposition final : private juce::ChangeListener
{
public:
    AppComposition();
    ~AppComposition() override;

    AppComposition(const AppComposition&) = delete;
    AppComposition& operator=(const AppComposition&) = delete;
    AppComposition(AppComposition&&) = delete;
    AppComposition& operator=(AppComposition&&) = delete;

    [[nodiscard]] std::unique_ptr<juce::Component> createMainComponent();

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void restoreDeviceSettings();
    void saveDeviceSettings();
    void updateSampleRate();

    io::StdFileSystem fileSystem_;
    io::DeviceSettingsStore deviceSettings_;
    engine::TestToneSource testTone_;
    JuceSampleLoader sampleLoader_;
    engine::KitBuilder kitBuilder_;
    engine::SampleEngine sampleEngine_;
    ui::KitPresenter kitPresenter_;
    AudioCallback audioCallback_;
    juce::AudioDeviceManager deviceManager_;
};

} // namespace drumprog::app
