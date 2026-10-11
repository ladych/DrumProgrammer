#include "app/AppComposition.h"

#include "DefaultKitData.h"
#include "LanguageData.h"
#include "app/Dialogs.h"
#include "app/MainComponent.h"
#include "i18n/Translate.h"
#include "ui/AboutInfo.h"
#include "ui/AudioStatus.h"

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace drumprog::app
{
namespace
{

constexpr double kTestToneFrequencyHz = 440.0;
constexpr float kTestToneGain = 0.2F;
constexpr int kNumOutputChannels = 2;
constexpr int kGlobalKitSaveIntervalMs = 1000;
#if defined(JUCE_ASIO) && JUCE_ASIO
constexpr bool kWithAsio = true;
#else
constexpr bool kWithAsio = false;
#endif

std::filesystem::path settingsFile(const char* name)
{
    const auto file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("DrumProgrammer")
                          .getChildFile(name);
    const std::string utf8 = file.getFullPathName().toStdString();
    // JUCE strings are UTF-8; std::filesystem only treats char8_t input as UTF-8 on every platform.
    return {std::u8string{utf8.begin(), utf8.end()}};
}

/// The default kit's files built into the program (resources/DefaultKit).
std::optional<std::string> defaultKitResource(std::string_view fileName)
{
    for (const auto* name : std::span{&DefaultKitData::namedResourceList[0],
                                      static_cast<std::size_t>(DefaultKitData::namedResourceListSize)})
    {
        if (fileName != DefaultKitData::getNamedResourceOriginalFilename(name))
            continue;
        int size = 0;
        const auto* data = DefaultKitData::getNamedResource(name, size);
        return std::string{data, static_cast<std::size_t>(size)};
    }
    return std::nullopt;
}

/// The language of the operating system, e.g. "de-DE".
std::string systemLanguage()
{
    return juce::SystemStats::getDisplayLanguage().toStdString();
}

/// The translation files built into the program (resources/Languages), with the saved language applied.
i18n::LanguageCatalog appliedLanguages(const io::SettingsStore& settings)
{
    i18n::LanguageCatalog catalog;
    for (const auto* name : std::span{&LanguageData::namedResourceList[0],
                                      static_cast<std::size_t>(LanguageData::namedResourceListSize)})
    {
        int size = 0;
        const auto* data = LanguageData::getNamedResource(name, size);
        const juce::String fileName{LanguageData::getNamedResourceOriginalFilename(name)};
        catalog.add(fileName.upToLastOccurrenceOf(".", false, false).toStdString(),
                    std::string{data, static_cast<std::size_t>(size)});
    }
    catalog.apply(
        catalog.resolve(settings.load().value_or(i18n::LanguageCatalog::kSystemLanguage), systemLanguage()));
    return catalog;
}

io::DefaultKit installedDefaultKit(io::IFileSystem& fileSystem)
{
    io::DefaultKit kit{fileSystem, settingsFile("DefaultKit"), &defaultKitResource};
    // A file that cannot be copied is reported as a missing sample of the global kit.
    kit.install();
    return kit;
}

} // namespace

AppComposition::AppComposition()
    : languageSettings_(fileSystem_, settingsFile("language.txt")),
      languages_(appliedLanguages(languageSettings_)),
      deviceSettings_(fileSystem_, settingsFile("audio-device.xml")),
      keymapSettings_(fileSystem_, settingsFile("keymap.txt")),
      recordOffsetSettings_(fileSystem_, settingsFile("recording-offset.txt")),
      globalKitSettings_(fileSystem_, settingsFile("kit.xml")), defaultKit_(installedDefaultKit(fileSystem_)),
      globalKit_(globalKitSettings_, fileSystem_, [this] { return defaultKit_.createKit(); }),
      testTone_(kTestToneFrequencyHz, kTestToneGain), kitBuilder_(sampleLoader_),
      kitPublisher_(kitBuilder_, sampleEngine_),
      playbackRenderer_(sequencer_, sampleEngine_, metronome_, backingTrackPlayer_),
      audioCallback_(testTone_, playbackRenderer_, snapshots_),
      deviceSupervisor_(deviceManager_, deviceSettings_, audioCallback_, kNumOutputChannels),
      projectFactory_(idGenerator_), project_(projectFactory_.createDefault()),
      snapshotPublisher_(project_, globalKit_.tree(), snapshots_, kitPublisher_),
      kitPresenter_(project_, globalKit_.tree(), undoManager_, kitPublisher_, sampleEngine_),
      takeRecorder_(project_, globalKit_.tree(), undoManager_),
      songTakeRecorder_(project_, globalKit_.tree(), undoManager_, idGenerator_),
      transportPresenter_(
          project_, sequencer_, metronome_, takeRecorder_, songTakeRecorder_, recordOffsetSettings_),
      tempoPresenter_(project_, undoManager_), keymap_(input::Keymap::fromText(keymapSettings_.load())),
      keymapPresenter_(keymap_, keymapSettings_, keyNames_, kitPresenter_),
      guiNoteSink_(sampleEngine_, &engine::steadyClockSeconds),
      keyboardInput_(keymap_, guiNoteSink_, inputActivity_),
      keyRouter_(keymapPresenter_, keyboardInput_, transportPresenter_), keyboardListener_(keyRouter_),
      midiNoteSink_(sampleEngine_, &engine::steadyClockSeconds),
      midiInputHandler_(midiNoteSink_, inputActivity_), midiInputCallback_(midiInputHandler_),
      inputLeds_(inputActivity_), activePattern_(project_),
      patternListPresenter_(project_, undoManager_, idGenerator_, activePattern_),
      pianoRollPresenter_(project_,
                          globalKit_.tree(),
                          undoManager_,
                          activePattern_,
                          [this](int slot) { return keymapPresenter_.keyLabel(slot); }),
      songTimelinePresenter_(project_, undoManager_, activePattern_),
      backingTrackPresenter_(project_, undoManager_, backingTrackLoader_, backingTrackPlayer_),
      mixPresenter_(project_, undoManager_),
      midiExportController_(project_, activePattern_, fileSystem_, midiDialogs_),
      midiImportController_(
          project_, globalKit_.tree(), undoManager_, idGenerator_, activePattern_, fileSystem_, midiDialogs_),
      projectRepository_(fileSystem_),
      documentController_(project_, undoManager_, projectRepository_, projectFactory_, documentView_),
      mainMenu_(
          documentController_,
          [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); },
          editActions(),
          patternActions(),
          songActions(),
          midiActions(),
          {.showKit = [this] { showKit(); },
           .resetToDefault = [this] { resetKitToDefault(); },
           .copyGlobalKitToProject = [this] { kitPresenter_.copyGlobalKitToProject(); },
           .useGlobalKit = [this] { kitPresenter_.useGlobalKit(); },
           .copyProjectKitToGlobal = [this] { kitPresenter_.copyProjectKitToGlobal(); },
           .usesProjectKit = [this] { return kitPresenter_.usesProjectKit(); }},
          {.showSettings = [this] { showAudioSettingsDialog(deviceManager_, dialogParent_); },
           .showKeymap = [this] { showKeymapDialog(keymapPresenter_, dialogParent_); },
           .loadBackingTrack = [this] { chooseBackingTrack(); },
           .removeBackingTrack = [this] { backingTrackPresenter_.remove(); },
           .hasBackingTrack = [this] { return backingTrackPresenter_.hasTrack(); }},
          {.showAbout = [this]
           { showAboutDialog(ui::makeAboutInfo(JUCE_APPLICATION_VERSION_STRING, kWithAsio), dialogParent_); },
           .languages =
               [this]
           {
               return languages_.choices(
                   languageSettings_.load().value_or(i18n::LanguageCatalog::kSystemLanguage),
                   systemLanguage());
           },
           .chooseLanguage = [this](const std::string& setting) { chooseLanguage(setting); }},
          {.toggleRecordArmed = [this] { transportPresenter_.toggleRecordArmed(); }})
{
    activePattern_.addOnChange([this](int index) { transportPresenter_.setActivePattern(index); });
    // A take recorded in the song mode opens in the piano roll (F-BT-08).
    transportPresenter_.setOnSongTake([this](int index) { activePattern_.select(index); });
    deviceSupervisor_.open();
    updateSampleRate();
    updateOutputLatency();
    deviceManager_.addAudioCallback(&audioCallback_);
    // Empty identifier: messages of every MIDI input enabled in the settings (F-IN-03).
    deviceManager_.addMidiInputDeviceCallback({}, &midiInputCallback_);
    deviceManager_.addChangeListener(this);
    documentController_.reportMissingGlobalKitSamples(globalKit_.missingSamples());
    startTimer(kGlobalKitSaveIntervalMs);
}

AppComposition::~AppComposition()
{
    stopTimer();
    // The kit dialog refers to the presenters.
    delete kitDialog_.getComponent();
    globalKit_.saveIfChanged();
    deviceManager_.removeChangeListener(this);
    deviceManager_.removeMidiInputDeviceCallback({}, &midiInputCallback_);
    deviceManager_.removeAudioCallback(&audioCallback_);
    deviceManager_.closeAudioDevice();
}

std::unique_ptr<juce::Component> AppComposition::createMainComponent()
{
    return std::make_unique<MainComponent>(
        deviceManager_,
        deviceSupervisor_,
        transportPresenter_,
        tempoPresenter_,
        patternListPresenter_,
        PatternDialogs{.add = [this] { addPattern(); },
                       .rename = [this](int index) { renamePattern(index); },
                       .setLength = [this](int index) { askPatternLength(index); },
                       .remove = [this](int index) { removePattern(index); }},
        songTimelinePresenter_,
        backingTrackPresenter_,
        backingTrackLoader_.formats(),
        [this] { chooseBackingTrack(); },
        mixPresenter_,
        pianoRollPresenter_,
        inputLeds_);
}

void AppComposition::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    saveDeviceSettings();
    updateSampleRate();
    updateOutputLatency();
}

void AppComposition::timerCallback()
{
    globalKit_.saveIfChanged();
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
        kitPublisher_.setDeviceSampleRate(device->getCurrentSampleRate());
}

void AppComposition::updateOutputLatency()
{
    // Recorded hits are moved back by the time until a block is heard (F-IN-08).
    ui::AudioDeviceInfo info;
    if (auto* device = deviceManager_.getCurrentAudioDevice())
    {
        info.open = true;
        info.sampleRate = device->getCurrentSampleRate();
        info.bufferSize = device->getCurrentBufferSizeSamples();
        info.outputLatencySamples = device->getOutputLatencyInSamples();
    }
    transportPresenter_.setOutputLatencyMs(info.open ? ui::outputLatencyMs(info) : 0.0);
}

void AppComposition::addPattern()
{
    askForNewPattern(juce::String::fromUTF8(patternListPresenter_.newName().c_str()),
                     ui::PatternListPresenter::kNewPatternBars,
                     [this](const std::string& name, int bars) { patternListPresenter_.add(name, bars); });
}

void AppComposition::renamePattern(int index)
{
    askForText(i18n::trJuce("Rename pattern"),
               i18n::trJuce("Name"),
               juce::String::fromUTF8(patternListPresenter_.name(index).c_str()),
               [this, index](const std::string& name) { patternListPresenter_.rename(index, name); });
}

void AppComposition::askPatternLength(int index)
{
    askForText(i18n::trJuce("Pattern length"),
               i18n::trJuce("Bars (1–64)"),
               juce::String(patternListPresenter_.lengthBars(index)),
               [this, index](const std::string& text)
               {
                   const int bars = juce::String::fromUTF8(text.c_str()).getIntValue();
                   if (bars > 0)
                       patternListPresenter_.setLengthBars(index, bars);
               });
}

void AppComposition::removePattern(int index)
{
    const auto question = patternListPresenter_.removeQuestion(index);
    if (question.empty())
    {
        patternListPresenter_.remove(index);
        return;
    }
    askToConfirm(i18n::trJuce("Delete pattern"),
                 juce::String::fromUTF8(question.c_str()),
                 i18n::trJuce("Delete"),
                 [this, index] { patternListPresenter_.remove(index); });
}

void AppComposition::chooseBackingTrack()
{
    backingTrackChooser_ = std::make_unique<juce::FileChooser>(
        i18n::trJuce("Load backing track"), juce::File{}, backingTrackLoader_.wildcardPattern());
    backingTrackChooser_->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();
            if (file == juce::File{})
                return;
            const auto utf8 = file.getFullPathName().toStdString();
            if (!backingTrackPresenter_.load(std::u8string{utf8.begin(), utf8.end()}))
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                       i18n::trJuce("Load backing track"),
                                                       i18n::trJuce("The file could not be read:") + "\n" +
                                                           file.getFullPathName());
        });
}

void AppComposition::showKit()
{
    if (kitDialog_ != nullptr)
    {
        kitDialog_->toFront(true);
        return;
    }
    kitDialog_ = showKitDialog(kitPresenter_,
                               keymapPresenter_,
                               sampleLoader_.wildcardPattern(),
                               {mainMenu_.keyMappings(), &keyboardListener_},
                               dialogParent_);
}

void AppComposition::resetKitToDefault()
{
    defaultKit_.install();
    kitPresenter_.resetToDefaultKit(defaultKit_.createKit());
}

void AppComposition::chooseLanguage(const std::string& setting)
{
    // An empty setting follows the system language, like no setting at all.
    languageSettings_.save(setting);
    const auto code = languages_.resolve(setting, systemLanguage());
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        juce::String::fromUTF8(languages_.translate(code, "Language").c_str()),
        juce::String::fromUTF8(languages_.restartNotice(setting, systemLanguage()).c_str()));
}

EditActions AppComposition::editActions()
{
    auto& roll = pianoRollPresenter_;
    return {.cut = [&roll] { roll.cut(); },
            .copy = [&roll] { roll.copy(); },
            .paste = [&roll] { roll.paste(); },
            .duplicate = [&roll] { roll.duplicate(); },
            .deleteSelection = [&roll] { roll.deleteSelection(); },
            .selectAll = [&roll] { roll.selectAll(); },
            .drawTool = [&roll] { roll.setTool(ui::PianoRollTool::draw); },
            .selectTool = [&roll] { roll.setTool(ui::PianoRollTool::select); },
            .eraseTool = [&roll] { roll.setTool(ui::PianoRollTool::erase); },
            .hasSelection = [&roll] { return roll.numSelected() > 0; },
            .canPaste = [&roll] { return roll.canPaste(); }};
}

PatternActions AppComposition::patternActions()
{
    auto& list = patternListPresenter_;
    return {.add = [this] { addPattern(); },
            .duplicate = [&list] { list.duplicate(list.selectedIndex()); },
            .rename = [this] { renamePattern(patternListPresenter_.selectedIndex()); },
            .setLength = [this] { askPatternLength(patternListPresenter_.selectedIndex()); },
            .remove = [this] { removePattern(patternListPresenter_.selectedIndex()); },
            .canRemove = [&list] { return list.canRemove(); }};
}

SongActions AppComposition::songActions()
{
    auto& song = songTimelinePresenter_;
    return {.insertPattern = [&song] { song.appendActivePattern(); },
            .removeBlock = [&song] { song.removeSelected(); },
            .clear = [&song] { song.clear(); },
            .hasSelection = [&song] { return song.selectedBlock() >= 0; },
            .isEmpty = [&song] { return song.isEmpty(); }};
}

MidiActions AppComposition::midiActions()
{
    return {.importFile = [this] { midiImportController_.importMidiFile(); },
            .exportFile = [this] { midiExportController_.exportMidi(); },
            .canExport = [this] { return midiExportController_.canExport(); }};
}

} // namespace drumprog::app
