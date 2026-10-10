#pragma once

#include "app/AudioCallback.h"
#include "app/JuceDocumentView.h"
#include "app/JuceMidiExportView.h"
#include "app/JuceSampleLoader.h"
#include "app/KeyboardInputGlue.h"
#include "app/MainMenu.h"
#include "app/MidiInputCallback.h"
#include "engine/KitBuilder.h"
#include "engine/KitPublisher.h"
#include "engine/Metronome.h"
#include "engine/PlaybackRenderer.h"
#include "engine/SampleEngine.h"
#include "engine/Sequencer.h"
#include "engine/TestToneSource.h"
#include "input/EngineNoteSinks.h"
#include "input/InputActivity.h"
#include "input/KeyboardInput.h"
#include "input/Keymap.h"
#include "input/MidiInputHandler.h"
#include "io/GlobalKit.h"
#include "io/ProjectRepository.h"
#include "io/SettingsStore.h"
#include "io/StdFileSystem.h"
#include "model/ProjectFactory.h"
#include "model/SnapshotPublisher.h"
#include "model/TakeRecorder.h"
#include "model/UuidGenerator.h"
#include "ui/ActivePattern.h"
#include "ui/DocumentController.h"
#include "ui/InputLedPresenter.h"
#include "ui/KeyRouter.h"
#include "ui/KeymapPresenter.h"
#include "ui/KitPresenter.h"
#include "ui/MidiExportController.h"
#include "ui/PatternListPresenter.h"
#include "ui/PianoRollPresenter.h"
#include "ui/SongTimelinePresenter.h"
#include "ui/TempoPresenter.h"
#include "ui/TransportPresenter.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>

namespace drumprog::app
{

/// Composition root (E-05): the only place where the object graph is created
/// and wired. Members are declared in dependency order, so they are destroyed
/// in reverse.
class AppComposition final : private juce::ChangeListener, private juce::Timer
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
    /// Key listener for the main window: computer keyboard as drum trigger (F-IN-01).
    [[nodiscard]] juce::KeyListener& keyboardListener() { return keyboardListener_; }
    /// Component the dialogs of the menu "Audio" are centred on.
    void setDialogParent(juce::Component* parent) { dialogParent_ = parent; }

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    /// Saves changes of the global kit.
    void timerCallback() override;
    void restoreDeviceSettings();
    void saveDeviceSettings();
    void updateSampleRate();
    void updateOutputLatency();
    void renamePattern(int index);
    void askPatternLength(int index);
    /// Asks first if the song uses the pattern (F-SO-05).
    void removePattern(int index);
    [[nodiscard]] EditActions editActions();
    [[nodiscard]] PatternActions patternActions();
    [[nodiscard]] SongActions songActions();

    io::StdFileSystem fileSystem_;
    io::SettingsStore deviceSettings_;
    io::SettingsStore keymapSettings_;
    io::SettingsStore recordOffsetSettings_;
    io::SettingsStore globalKitSettings_;
    io::GlobalKit globalKit_;
    engine::TestToneSource testTone_;
    JuceSampleLoader sampleLoader_;
    engine::KitBuilder kitBuilder_;
    engine::SampleEngine sampleEngine_;
    engine::KitPublisher kitPublisher_;
    // Transport and sequencer (AP4): the audio thread reads snapshots_ through the renderer.
    model::ProjectSnapshotExchange snapshots_;
    engine::Sequencer sequencer_;
    engine::Metronome metronome_;
    engine::PlaybackRenderer playbackRenderer_;
    AudioCallback audioCallback_;
    juce::AudioDeviceManager deviceManager_;

    // Project model (AP1). It is the only source of the notes, and of the kit together with the global
    // kit: the SnapshotPublisher hands every change to the engine.
    model::UuidGenerator idGenerator_;
    model::ProjectFactory projectFactory_;
    juce::UndoManager undoManager_;
    juce::ValueTree project_;
    model::SnapshotPublisher snapshotPublisher_;
    ui::KitPresenter kitPresenter_;
    model::TakeRecorder takeRecorder_;
    ui::TransportPresenter transportPresenter_;
    ui::TempoPresenter tempoPresenter_;

    // Live input (AP3): computer keyboard on the GUI thread, MIDI on the MIDI thread, each with its
    // own lock-free queue into the engine.
    input::InputActivity inputActivity_;
    input::Keymap keymap_;
    NativeKeyNames keyNames_;
    ui::KeymapPresenter keymapPresenter_;
    input::GuiNoteSink guiNoteSink_;
    input::KeyboardInput keyboardInput_;
    ui::KeyRouter keyRouter_;
    KeyboardListener keyboardListener_;
    input::MidiNoteSink midiNoteSink_;
    input::MidiInputHandler midiInputHandler_;
    MidiInputCallback midiInputCallback_;
    ui::InputLedPresenter inputLeds_;

    // Editing (AP5): the active pattern is shown in the piano roll and played by the transport.
    ui::ActivePattern activePattern_;
    ui::PatternListPresenter patternListPresenter_;
    ui::PianoRollPresenter pianoRollPresenter_;
    // Song (AP7): blocks reference patterns; the transport plays the timeline in the song mode.
    ui::SongTimelinePresenter songTimelinePresenter_;

    // MIDI export (AP6) of the active pattern.
    JuceMidiExportView midiExportView_;
    ui::MidiExportController midiExportController_;

    io::ProjectRepository projectRepository_;
    JuceDocumentView documentView_;
    ui::DocumentController documentController_;
    juce::Component* dialogParent_ = nullptr;
    MainMenu mainMenu_;
};

} // namespace drumprog::app
