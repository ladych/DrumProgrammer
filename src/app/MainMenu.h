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

/// Editing commands of the piano roll in the menu "Bearbeiten" (F-PR-02, F-PR-03, F-PR-05).
struct EditActions
{
    std::function<void()> cut;
    std::function<void()> copy;
    std::function<void()> paste;
    std::function<void()> duplicate;
    std::function<void()> deleteSelection;
    std::function<void()> selectAll;
    std::function<void()> drawTool;
    std::function<void()> selectTool;
    std::function<void()> eraseTool;
    std::function<bool()> hasSelection;
    std::function<bool()> canPaste;
};

/// Actions of the menu "Pattern" on the active pattern (F-SO-01, F-PR-09).
struct PatternActions
{
    std::function<void()> add;
    std::function<void()> duplicate;
    std::function<void()> rename;
    std::function<void()> setLength;
    std::function<void()> remove;
    std::function<bool()> canRemove;
};

/// Transport commands that only have a shortcut, no menu entry (F-TR-07).
struct TransportActions
{
    std::function<void()> toggleRecordArmed;
};

/// Humble object (E-03): menus "Datei", "Bearbeiten", "Pattern", "Kit" and "Audio" with their shortcuts
/// (Pflichtenheft 6.2), forwarding every command to the DocumentController or the actions. Shortcuts use
/// Strg only, so letter keys stay free for drum triggers (6.4), except Entf; the space bar for play/stop
/// is handled by ui::KeyRouter.
class MainMenu final : public juce::MenuBarModel, public juce::ApplicationCommandTarget
{
public:
    MainMenu(ui::DocumentController& document,
             std::function<void()> quit,
             EditActions edit,
             PatternActions pattern,
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
    [[nodiscard]] std::function<void()> editActionFor(juce::CommandID commandID) const;
    [[nodiscard]] std::function<void()> patternActionFor(juce::CommandID commandID) const;
    [[nodiscard]] std::function<void()> kitActionFor(juce::CommandID commandID) const;
    /// Whether the command can run now; empty for commands that always can.
    [[nodiscard]] std::function<bool()> activeCheckFor(juce::CommandID commandID) const;

    ui::DocumentController& document_;
    std::function<void()> quit_;
    EditActions edit_;
    PatternActions pattern_;
    KitMenuActions kit_;
    AudioMenuActions audio_;
    TransportActions transport_;
    juce::ApplicationCommandManager commands_;
};

} // namespace drumprog::app
