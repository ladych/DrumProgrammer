#include "app/Dialogs.h"

#include "app/KeymapDialog.h"

#include <memory>
#include <utility>

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

void askForText(const juce::String& title,
                const juce::String& label,
                const juce::String& initialText,
                std::function<void(const std::string&)> onOk)
{
    auto* window = new juce::AlertWindow(title, label, juce::MessageBoxIconType::NoIcon);
    window->addTextEditor("text", initialText);
    window->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Abbrechen", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    // The window deletes itself after the callback (deleteWhenDismissed).
    window->enterModalState(true,
                            juce::ModalCallbackFunction::create(
                                [window, onOk = std::move(onOk)](int result)
                                {
                                    if (result == 1)
                                        onOk(window->getTextEditorContents("text").toStdString());
                                }),
                            true);
}

} // namespace drumprog::app
