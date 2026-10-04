#pragma once

#include "ui/DocumentController.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace drumprog::app
{

/// Actions of the menu "Audio" that open dialogs.
struct AudioMenuActions
{
    std::function<void()> showSettings;
    std::function<void()> showKeymap;
};

/// Actions of the menu "Kit": where the project's kit comes from.
struct KitMenuActions
{
    std::function<void()> copyGlobalKitToProject;
    std::function<void()> useGlobalKit;
    std::function<void()> copyProjectKitToGlobal;
    std::function<bool()> usesProjectKit;
};

/// Transport commands that only have a shortcut, no menu entry (F-TR-07).
struct TransportActions
{
    std::function<void()> toggleRecordArmed;
};

/// Humble object (E-03): menus "Datei", "Bearbeiten", "Kit" and "Audio" with their shortcuts (Pflichtenheft
/// 6.2), forwarding every command to the DocumentController or the actions. Shortcuts use Strg only,
/// so letter keys stay free for drum triggers (6.4); the space bar for play/stop is handled by
/// ui::KeyRouter.
class MainMenu final : public juce::MenuBarModel, public juce::ApplicationCommandTarget
{
public:
    MainMenu(ui::DocumentController& document,
             std::function<void()> quit,
             KitMenuActions kit,
             AudioMenuActions audio,
             TransportActions transport);
    ~MainMenu() override;

    MainMenu(const MainMenu&) = delete;
    MainMenu& operator=(const MainMenu&) = delete;
    MainMenu(MainMenu&&) = delete;
    MainMenu& operator=(MainMenu&&) = delete;

    /// Key listener for the main window.
    [[nodiscard]] juce::KeyListener* keyMappings() { return commands_.getKeyMappings(); }

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    void menuItemSelected(int /*menuItemID*/, int /*topLevelMenuIndex*/) override {}

    juce::ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

private:
    [[nodiscard]] std::function<void()> actionFor(juce::CommandID commandID);
    [[nodiscard]] std::function<void()> kitActionFor(juce::CommandID commandID) const;

    ui::DocumentController& document_;
    std::function<void()> quit_;
    KitMenuActions kit_;
    AudioMenuActions audio_;
    TransportActions transport_;
    juce::ApplicationCommandManager commands_;
};

} // namespace drumprog::app
