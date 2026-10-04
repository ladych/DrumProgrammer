#pragma once

#include "app/NativeKeyboard.h"
#include "input/KeyNames.h"
#include "ui/KeyRouter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>
#include <string>

namespace drumprog::app
{

/// Humble object (E-03): key labels of the current keyboard layout, US labels where the system has
/// none.
class NativeKeyNames final : public input::IKeyNames
{
public:
    [[nodiscard]] std::string nameOf(int scancode) const override
    {
        if (const auto label = native::keyLabel(scancode))
            return juce::String::fromUTF8(label->c_str()).toUpperCase().toStdString();
        return fallback_.nameOf(scancode);
    }

private:
    input::UsKeyNames fallback_;
};

/// Physical key of a JUCE key press.
[[nodiscard]] inline std::optional<int> scancodeOf(const juce::KeyPress& key)
{
    return native::scancodeForKeyCode(key.getKeyCode());
}

/// Humble object (E-03): key listener of the main window, forwarding presses and releases to the
/// ui::KeyRouter (F-IN-01, F-IN-06). Keys a focused component used (e.g. typing into a text field)
/// never get here; text fields that ignore a key are reported as focused so it does not trigger.
class KeyboardListener final : public juce::KeyListener
{
public:
    explicit KeyboardListener(ui::KeyRouter& router) : router_(router) {}

    bool keyPressed(const juce::KeyPress& key, juce::Component* /*originatingComponent*/) override
    {
        const auto scancode = scancodeOf(key);
        if (!scancode)
            return false;
        const auto modifiers = key.getModifiers();
        const bool textFieldHasFocus =
            dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent()) != nullptr;
        return router_.keyDown(
            *scancode,
            {.shift = modifiers.isShiftDown(),
             .commandOrAlt = modifiers.isCommandDown() || modifiers.isCtrlDown() || modifiers.isAltDown()},
            textFieldHasFocus);
    }

    bool keyStateChanged(bool /*isKeyDown*/, juce::Component* /*originatingComponent*/) override
    {
        router_.releaseKeysNotDown([](int scancode) { return native::isScancodeDown(scancode); });
        return false;
    }

private:
    ui::KeyRouter& router_;
};

} // namespace drumprog::app
