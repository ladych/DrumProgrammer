#include "app/MainMenu.h"

#include <array>
#include <utility>

namespace drumprog::app
{
namespace
{

namespace command
{
constexpr juce::CommandID fileNew = 0x1001;
constexpr juce::CommandID fileOpen = 0x1002;
constexpr juce::CommandID fileSave = 0x1003;
constexpr juce::CommandID fileSaveAs = 0x1004;
constexpr juce::CommandID fileQuit = 0x1005;
constexpr juce::CommandID editUndo = 0x2001;
constexpr juce::CommandID editRedo = 0x2002;
} // namespace command

constexpr int kCtrl = juce::ModifierKeys::commandModifier;
constexpr int kCtrlShift = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;

struct CommandSpec
{
    juce::CommandID id;
    const char* name; ///< UTF-8
    const char* description;
    const char* category;
    char key;
    int modifiers;
};

constexpr std::array kCommands{
    CommandSpec{command::fileNew, "Neu", "Neues Projekt anlegen", "Datei", 'n', kCtrl},
    CommandSpec{command::fileOpen, "Öffnen...", "Projekt öffnen", "Datei", 'o', kCtrl},
    CommandSpec{command::fileSave, "Speichern", "Projekt speichern", "Datei", 's', kCtrl},
    CommandSpec{command::fileSaveAs,
                "Speichern unter...",
                "Projekt unter neuem Namen speichern",
                "Datei",
                's',
                kCtrlShift},
    CommandSpec{command::fileQuit, "Beenden", "Programm beenden", "Datei", 'q', kCtrl},
    CommandSpec{
        command::editUndo, "Rückgängig", "Letzte Änderung rückgängig machen", "Bearbeiten", 'z', kCtrl},
    CommandSpec{command::editRedo,
                "Wiederholen",
                "Rückgängig gemachte Änderung wiederholen",
                "Bearbeiten",
                'y',
                kCtrl},
};

const CommandSpec* findCommand(juce::CommandID id)
{
    for (const auto& spec : kCommands)
        if (spec.id == id)
            return &spec;
    return nullptr;
}

} // namespace

MainMenu::MainMenu(ui::DocumentController& document, std::function<void()> quit)
    : document_(document), quit_(std::move(quit))
{
    commands_.registerAllCommandsForTarget(this);
    commands_.setFirstCommandTarget(this);
    setApplicationCommandManagerToWatch(&commands_);
}

MainMenu::~MainMenu()
{
    setApplicationCommandManagerToWatch(nullptr);
}

juce::StringArray MainMenu::getMenuBarNames()
{
    return {"Datei", "Bearbeiten"};
}

juce::PopupMenu MainMenu::getMenuForIndex(int menuIndex, const juce::String& /*menuName*/)
{
    juce::PopupMenu menu;
    if (menuIndex == 0)
    {
        menu.addCommandItem(&commands_, command::fileNew);
        menu.addCommandItem(&commands_, command::fileOpen);
        menu.addSeparator();
        menu.addCommandItem(&commands_, command::fileSave);
        menu.addCommandItem(&commands_, command::fileSaveAs);
        menu.addSeparator();
        menu.addCommandItem(&commands_, command::fileQuit);
    }
    else
    {
        menu.addCommandItem(&commands_, command::editUndo);
        menu.addCommandItem(&commands_, command::editRedo);
    }
    return menu;
}

void MainMenu::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    for (const auto& spec : kCommands)
        commands.add(spec.id);
}

void MainMenu::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    const auto* spec = findCommand(commandID);
    if (spec == nullptr)
        return;

    result.setInfo(
        juce::String::fromUTF8(spec->name), juce::String::fromUTF8(spec->description), spec->category, 0);
    result.addDefaultKeypress(spec->key, spec->modifiers);
    if (commandID == command::editUndo)
        result.setActive(document_.canUndo());
    if (commandID == command::editRedo)
    {
        result.addDefaultKeypress('z', kCtrlShift);
        result.setActive(document_.canRedo());
    }
}

bool MainMenu::perform(const InvocationInfo& info)
{
    switch (info.commandID)
    {
    case command::fileNew:
        document_.newProject();
        return true;
    case command::fileOpen:
        document_.open();
        return true;
    case command::fileSave:
        document_.save();
        return true;
    case command::fileSaveAs:
        document_.saveAs();
        return true;
    case command::fileQuit:
        quit_();
        return true;
    case command::editUndo:
        document_.undo();
        return true;
    case command::editRedo:
        document_.redo();
        return true;
    default:
        return false;
    }
}

} // namespace drumprog::app
