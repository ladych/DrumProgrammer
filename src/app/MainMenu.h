#pragma once

#include "ui/DocumentController.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace drumprog::app
{

/// Humble object (E-03): menus "Datei" and "Bearbeiten" with their shortcuts (Pflichtenheft 6.2),
/// forwarding every command to the DocumentController. Shortcuts use Strg only, so letter keys stay
/// free for drum triggers (6.4).
class MainMenu final : public juce::MenuBarModel, public juce::ApplicationCommandTarget
{
public:
    MainMenu(ui::DocumentController& document, std::function<void()> quit);
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
    ui::DocumentController& document_;
    std::function<void()> quit_;
    juce::ApplicationCommandManager commands_;
};

} // namespace drumprog::app
