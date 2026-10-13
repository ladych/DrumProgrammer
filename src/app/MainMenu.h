#pragma once

#include "i18n/LanguageCatalog.h"
#include "ui/DocumentController.h"
#include "ui/ThemeSetting.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <string>
#include <vector>

namespace drumprog::app
{

/// Actions of the menu "Audio" that open dialogs.
struct AudioMenuActions
{
    std::function<void()> showSettings;
    std::function<void()> showKeymap;
    std::function<void()> loadBackingTrack;
    std::function<void()> removeBackingTrack;
    std::function<bool()> hasBackingTrack;
};

/// Actions of the menu "Kit": the kit dialog, where the project's kit comes from and the default kit.
struct KitMenuActions
{
    std::function<void()> showKit;
    std::function<void()> resetToDefault;
    std::function<void()> copyGlobalKitToProject;
    std::function<void()> useGlobalKit;
    std::function<void()> copyProjectKitToGlobal;
    std::function<bool()> usesProjectKit;
};

/// Actions of the menu "Hilfe", with the submenus "Sprache" (AP15) and "Farbschema" (AP16).
struct HelpActions
{
    std::function<void()> showAbout;
    std::function<std::vector<i18n::LanguageChoice>()> languages;
    std::function<void(const std::string& setting)> chooseLanguage;
    std::function<std::vector<ui::ThemeChoice>()> themes;
    std::function<void(const std::string& setting)> chooseTheme;
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

/// Actions of the menu "Song" (Pflichtenheft 6.2, F-SO-02, F-SO-03).
struct SongActions
{
    std::function<void()> insertPattern;
    std::function<void()> removeBlock;
    std::function<void()> clear;
    std::function<bool()> hasSelection;
    std::function<bool()> isEmpty;
};

/// Actions of the menu "MIDI" (F-MI-01 to 05), also behind the toolbar buttons.
struct MidiActions
{
    std::function<void()> importFile;
    std::function<void()> exportFile;
    std::function<bool()> canExport;
};

/// Transport commands that only have a shortcut, no menu entry (F-TR-07).
struct TransportActions
{
    std::function<void()> toggleRecordArmed;
};

/// Humble object (E-03): menus "Datei", "Bearbeiten", "Pattern", "Song", "MIDI", "Kit", "Audio" and "Hilfe"
/// with their shortcuts (Pflichtenheft 6.2), forwarding every command to the DocumentController or the
/// actions. Shortcuts use Strg only, so letter keys stay free for drum triggers (6.4), except Entf; the space
/// bar for play/stop is handled by ui::KeyRouter.
class MainMenu final : public juce::MenuBarModel, public juce::ApplicationCommandTarget
{
public:
    MainMenu(ui::DocumentController& document,
             std::function<void()> quit,
             EditActions edit,
             PatternActions pattern,
             SongActions song,
             MidiActions midi,
             KitMenuActions kit,
             AudioMenuActions audio,
             HelpActions help,
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
    /// Entries of the submenus "Sprache" and "Farbschema"; the commands go through perform().
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    juce::ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

private:
    [[nodiscard]] std::function<void()> actionFor(juce::CommandID commandID);
    [[nodiscard]] std::function<void()> editActionFor(juce::CommandID commandID) const;
    [[nodiscard]] std::function<void()> patternActionFor(juce::CommandID commandID) const;
    [[nodiscard]] std::function<void()> songActionFor(juce::CommandID commandID) const;
    [[nodiscard]] std::function<void()> kitActionFor(juce::CommandID commandID) const;
    [[nodiscard]] std::function<void()> audioActionFor(juce::CommandID commandID) const;
    /// Whether the command can run now; empty for commands that always can.
    [[nodiscard]] std::function<bool()> activeCheckFor(juce::CommandID commandID) const;
    [[nodiscard]] std::function<bool()> projectCheckFor(juce::CommandID commandID) const;

    ui::DocumentController& document_;
    std::function<void()> quit_;
    EditActions edit_;
    PatternActions pattern_;
    SongActions song_;
    MidiActions midi_;
    KitMenuActions kit_;
    AudioMenuActions audio_;
    HelpActions help_;
    TransportActions transport_;
    juce::ApplicationCommandManager commands_;
};

} // namespace drumprog::app
