#include "app/Dialogs.h"

#include "app/AboutDialog.h"
#include "app/KeymapDialog.h"
#include "app/KitPanel.h"
#include "app/Theme.h"
#include "i18n/Translate.h"
#include "ui/PatternListPresenter.h"

#include <memory>
#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kKitDialogWidth = 760;
constexpr int kKitDialogHeight = 520;

juce::DialogWindow* launch(std::unique_ptr<juce::Component> content,
                           const juce::String& title,
                           juce::Component* centreAround,
                           bool resizable = false)
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogBackgroundColour =
        content->getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.content.setOwned(content.release());
    options.dialogTitle = title;
    options.componentToCentreAround = centreAround;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = resizable;
    return options.launchAsync();
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
    launch(std::move(selector), i18n::trJuce("Audio/MIDI settings"), centreAround);
}

void showKeymapDialog(ui::KeymapPresenter& presenter, juce::Component* centreAround)
{
    launch(std::make_unique<KeymapDialog>(presenter), i18n::trJuce("Keyboard mapping"), centreAround);
}

juce::DialogWindow* showKitDialog(ui::KitPresenter& kit,
                                  ui::KeymapPresenter& keymap,
                                  const juce::String& sampleWildcard,
                                  const std::vector<juce::KeyListener*>& keyListeners,
                                  juce::Component* centreAround)
{
    auto panel = std::make_unique<KitPanel>(kit, keymap, sampleWildcard);
    panel->setSize(kKitDialogWidth, kKitDialogHeight);
    auto* window = launch(std::move(panel), i18n::trJuce("Kit"), centreAround, true);
    for (auto* listener : keyListeners)
        window->addKeyListener(listener);
    return window;
}

void showAboutDialog(const ui::AboutInfo& info, juce::Component* centreAround)
{
    launch(std::make_unique<AboutDialog>(info), i18n::trJuce("About Drum Programmer"), centreAround, true);
}

void askForText(const juce::String& title,
                const juce::String& label,
                const juce::String& initialText,
                std::function<void(const std::string&)> onOk)
{
    auto* window = new juce::AlertWindow(title, label, juce::MessageBoxIconType::NoIcon);
    window->addTextEditor("text", initialText);
    window->addButton(i18n::trJuce("OK"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton(i18n::trJuce("Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
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

void markInput(juce::TextEditor& editor, bool valid)
{
    // A valid input takes the colours of the look and feel, so it follows a change of the theme (AP16).
    if (valid)
    {
        editor.removeColour(juce::TextEditor::outlineColourId);
        editor.removeColour(juce::TextEditor::focusedOutlineColourId);
    }
    else
    {
        editor.setColour(juce::TextEditor::outlineColourId, theme::colours().record);
        editor.setColour(juce::TextEditor::focusedOutlineColourId, theme::colours().record);
    }
    // applyColourToAllText() also sets the editor's own text colour, so the valid one comes from the look and
    // feel.
    editor.applyColourToAllText(valid ? editor.getLookAndFeel().findColour(juce::TextEditor::textColourId)
                                      : theme::colours().record);
    editor.repaint();
}

void askForNewPattern(const juce::String& name,
                      int bars,
                      std::function<void(const std::string& name, int bars)> onOk)
{
    auto* window = new juce::AlertWindow(i18n::trJuce("New pattern"), {}, juce::MessageBoxIconType::NoIcon);
    window->addTextEditor("name", name, i18n::trJuce("Name"));
    window->addTextEditor("bars", juce::String(bars), i18n::trJuce("Length in bars (1–64)"));
    window->addButton(i18n::trJuce("OK"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton(i18n::trJuce("Cancel"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    auto* barsEditor = window->getTextEditor("bars");
    barsEditor->setInputRestrictions(2, "0123456789");
    barsEditor->onTextChange = [window, barsEditor]
    {
        const bool valid =
            ui::PatternListPresenter::parseBars(barsEditor->getText().toStdString()).has_value();
        markInput(*barsEditor, valid);
        window->getButton(i18n::trJuce("OK"))->setEnabled(valid);
    };
    // The window deletes itself after the callback (deleteWhenDismissed).
    window->enterModalState(true,
                            juce::ModalCallbackFunction::create(
                                [window, onOk = std::move(onOk)](int result)
                                {
                                    const auto length = ui::PatternListPresenter::parseBars(
                                        window->getTextEditorContents("bars").toStdString());
                                    if (result == 1 && length)
                                        onOk(window->getTextEditorContents("name").toStdString(), *length);
                                }),
                            true);
}

void askToConfirm(const juce::String& title,
                  const juce::String& question,
                  const juce::String& yesButton,
                  std::function<void()> onYes)
{
    const auto options = juce::MessageBoxOptions::makeOptionsOkCancel(
        juce::MessageBoxIconType::QuestionIcon, title, question, yesButton, i18n::trJuce("Cancel"));
    juce::AlertWindow::showAsync(options,
                                 [onYes = std::move(onYes)](int result)
                                 {
                                     if (result == 1)
                                         onYes();
                                 });
}

} // namespace drumprog::app
