#include "app/Dialogs.h"

#include "app/KeymapDialog.h"

#include <memory>

namespace drumprog::app
{
namespace
{

void launch(std::unique_ptr<juce::Component> content,
            const juce::String& title,
            juce::Component* centreAround)
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogBackgroundColour =
        content->getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.content.setOwned(content.release());
    options.dialogTitle = title;
    options.componentToCentreAround = centreAround;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

} // namespace

void showAudioSettingsDialog(juce::AudioDeviceManager& deviceManager, juce::Component* centreAround)
{
    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager,
                                                                         0,
                                                                         0, // audio inputs
                                                                         0,
                                                                         2,     // audio outputs
                                                                         true,  // MIDI inputs
                                                                         false, // MIDI output
                                                                         true,  // stereo pairs
                                                                         false);
    selector->setSize(560, 440);
    launch(std::move(selector), "Audio/MIDI-Einstellungen", centreAround);
}

void showKeymapDialog(ui::KeymapPresenter& presenter, juce::Component* centreAround)
{
    launch(std::make_unique<KeymapDialog>(presenter), "Tastatur-Mapping", centreAround);
}

} // namespace drumprog::app
