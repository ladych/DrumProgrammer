#include "app/AppComposition.h"

#include "app/Dialogs.h"
#include "app/MainComponent.h"
#include "ui/AudioStatus.h"

#include <filesystem>
#include <string>

namespace drumprog::app
{
namespace
{

constexpr double kTestToneFrequencyHz = 440.0;
constexpr float kTestToneGain = 0.2F;
constexpr int kNumOutputChannels = 2;
constexpr int kGlobalKitSaveIntervalMs = 1000;

std::filesystem::path settingsFile(const char* name)
{
    const auto file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("DrumProgrammer")
                          .getChildFile(name);
    const std::string utf8 = file.getFullPathName().toStdString();
    // JUCE strings are UTF-8; std::filesystem only treats char8_t input as UTF-8 on every platform.
    return {std::u8string{utf8.begin(), utf8.end()}};
}

} // namespace

AppComposition::AppComposition()
    : deviceSettings_(fileSystem_, settingsFile("audio-device.xml")),
      keymapSettings_(fileSystem_, settingsFile("keymap.txt")),
      recordOffsetSettings_(fileSystem_, settingsFile("recording-offset.txt")),
      globalKitSettings_(fileSystem_, settingsFile("kit.xml")), globalKit_(globalKitSettings_, fileSystem_),
      testTone_(kTestToneFrequencyHz, kTestToneGain), kitBuilder_(sampleLoader_),
      kitPublisher_(kitBuilder_, sampleEngine_), playbackRenderer_(sequencer_, sampleEngine_, metronome_),
      audioCallback_(testTone_, playbackRenderer_, snapshots_), projectFactory_(idGenerator_),
      project_(projectFactory_.createDefault()),
      snapshotPublisher_(project_, globalKit_.tree(), snapshots_, kitPublisher_),
      kitPresenter_(project_, globalKit_.tree(), undoManager_, kitPublisher_, sampleEngine_),
      takeRecorder_(project_, globalKit_.tree(), undoManager_),
      transportPresenter_(project_, sequencer_, metronome_, takeRecorder_, recordOffsetSettings_),
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
      midiExportController_(project_, activePattern_, fileSystem_, midiExportView_),
      projectRepository_(fileSystem_),
      documentController_(project_, undoManager_, projectRepository_, projectFactory_, documentView_),
      mainMenu_(documentController_,
                [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); },
                editActions(),
                patternActions(),
                {.exportPattern = [this] { midiExportController_.exportActivePattern(); },
                 .canExportPattern = [this] { return midiExportController_.canExportPattern(); }},
                {.copyGlobalKitToProject = [this] { kitPresenter_.copyGlobalKitToProject(); },
                 .useGlobalKit = [this] { kitPresenter_.useGlobalKit(); },
                 .copyProjectKitToGlobal = [this] { kitPresenter_.copyProjectKitToGlobal(); },
                 .usesProjectKit = [this] { return kitPresenter_.usesProjectKit(); }},
                {.showSettings = [this] { showAudioSettingsDialog(deviceManager_, dialogParent_); },
                 .showKeymap = [this] { showKeymapDialog(keymapPresenter_, dialogParent_); }},
                {.toggleRecordArmed = [this] { transportPresenter_.toggleRecordArmed(); }})
{
    activePattern_.addOnChange([this](int index) { transportPresenter_.setActivePattern(index); });
    restoreDeviceSettings();
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
    globalKit_.saveIfChanged();
    deviceManager_.removeChangeListener(this);
    deviceManager_.removeMidiInputDeviceCallback({}, &midiInputCallback_);
    deviceManager_.removeAudioCallback(&audioCallback_);
    deviceManager_.closeAudioDevice();
}

std::unique_ptr<juce::Component> AppComposition::createMainComponent()
{
    return std::make_unique<MainComponent>(
        testTone_,
        deviceManager_,
        transportPresenter_,
        tempoPresenter_,
        kitPresenter_,
        patternListPresenter_,
        PatternDialogs{.rename = [this](int index) { renamePattern(index); },
                       .setLength = [this](int index) { askPatternLength(index); }},
        pianoRollPresenter_,
        keymapPresenter_,
        inputLeds_,
        sampleLoader_.wildcardPattern());
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

void AppComposition::restoreDeviceSettings()
{
    const auto savedState = deviceSettings_.load();
    const auto xml = savedState ? juce::parseXML(juce::String::fromUTF8(savedState->c_str())) : nullptr;
    const auto error = deviceManager_.initialise(0, kNumOutputChannels, xml.get(), true);
    if (error.isNotEmpty())
        juce::Logger::writeToLog("Audio device could not be opened: " + error);
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

void AppComposition::renamePattern(int index)
{
    askForText("Pattern umbenennen",
               "Name",
               juce::String::fromUTF8(patternListPresenter_.name(index).c_str()),
               [this, index](const std::string& name) { patternListPresenter_.rename(index, name); });
}

void AppComposition::askPatternLength(int index)
{
    askForText(juce::String::fromUTF8("Pattern-L\xc3\xa4nge"),
               juce::String::fromUTF8("Takte (1\xe2\x80\x93"
                                      "64)"),
               juce::String(patternListPresenter_.lengthBars(index)),
               [this, index](const std::string& text)
               {
                   const int bars = juce::String::fromUTF8(text.c_str()).getIntValue();
                   if (bars > 0)
                       patternListPresenter_.setLengthBars(index, bars);
               });
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
    return {.add = [&list] { list.add(); },
            .duplicate = [&list] { list.duplicate(list.selectedIndex()); },
            .rename = [this] { renamePattern(patternListPresenter_.selectedIndex()); },
            .setLength = [this] { askPatternLength(patternListPresenter_.selectedIndex()); },
            .remove = [&list] { list.remove(list.selectedIndex()); },
            .canRemove = [&list] { return list.canRemove(); }};
}

} // namespace drumprog::app
