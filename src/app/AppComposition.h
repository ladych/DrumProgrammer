#pragma once

#include "app/AudioCallback.h"
#include "app/JuceDocumentView.h"
#include "app/JuceSampleLoader.h"
#include "app/MainMenu.h"
#include "engine/KitBuilder.h"
#include "engine/SampleEngine.h"
#include "engine/TestToneSource.h"
#include "io/DeviceSettingsStore.h"
#include "io/ProjectRepository.h"
#include "io/StdFileSystem.h"
#include "model/ProjectFactory.h"
#include "model/SnapshotPublisher.h"
#include "model/UuidGenerator.h"
#include "ui/DocumentController.h"
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
    [[nodiscard]] ui::DocumentController& documentController() { return documentController_; }
    [[nodiscard]] JuceDocumentView& documentView() { return documentView_; }
    [[nodiscard]] MainMenu& mainMenu() { return mainMenu_; }

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

    // Project model (AP1). The audio thread will read snapshots_ from the sequencer on (AP4).
    model::UuidGenerator idGenerator_;
    model::ProjectFactory projectFactory_;
    juce::UndoManager undoManager_;
    juce::ValueTree project_;
    model::ProjectSnapshotExchange snapshots_;
    model::SnapshotPublisher snapshotPublisher_;
    io::ProjectRepository projectRepository_;
    JuceDocumentView documentView_;
    ui::DocumentController documentController_;
    MainMenu mainMenu_;
};

} // namespace drumprog::app
