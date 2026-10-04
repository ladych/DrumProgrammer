#include "app/NativeKeyboard.h"

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <array>
#include <cstdint>

namespace drumprog::app::native
{
namespace
{

/// X11 keycodes are the Linux evdev key codes plus 8; evdev codes equal the set-1 scancodes of the
/// main key block.
constexpr int kKeycodeOffset = 8;
/// juce::KeyPress marks non-character keys with this bit on Linux (juce_XWindowSystem_linux.cpp).
constexpr int kJuceExtendedKey = 0x10000000;
constexpr KeySym kUnicodeKeysym = 0x01000000;

/// Our own connection for keyboard queries, opened on first use and kept for the program's lifetime.
/// Only the GUI thread uses it.
Display* display()
{
    static Display* const connection = XOpenDisplay(nullptr);
    return connection;
}

KeySym keysymForKeyCode(int keyCode)
{
    if ((keyCode & kJuceExtendedKey) != 0)
        return 0xff00 | static_cast<KeySym>(keyCode & 0xff);
    if (keyCode < 0x20) // tab, return, escape, backspace
        return 0xff00 | static_cast<KeySym>(keyCode);
    if (keyCode < 0x100)
        return static_cast<KeySym>(keyCode);
    return kUnicodeKeysym | static_cast<KeySym>(keyCode);
}

std::string toUtf8(char32_t c)
{
    std::string utf8;
    if (c < 0x80)
    {
        utf8 += static_cast<char>(c);
    }
    else if (c < 0x800)
    {
        utf8 += static_cast<char>(0xC0 | (c >> 6));
        utf8 += static_cast<char>(0x80 | (c & 0x3F));
    }
    else
    {
        utf8 += static_cast<char>(0xE0 | (c >> 12));
        utf8 += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        utf8 += static_cast<char>(0x80 | (c & 0x3F));
    }
    return utf8;
}

} // namespace

std::optional<int> scancodeForKeyCode(int juceKeyCode)
{
    auto* connection = display();
    if (connection == nullptr)
        return std::nullopt;
    const KeyCode keycode = XKeysymToKeycode(connection, keysymForKeyCode(juceKeyCode));
    if (keycode < kKeycodeOffset)
        return std::nullopt;
    return keycode - kKeycodeOffset;
}

bool isScancodeDown(int scancode)
{
    auto* connection = display();
    const int keycode = scancode + kKeycodeOffset;
    if (connection == nullptr || keycode > 255)
        return false;
    std::array<char, 32> keys{};
    XQueryKeymap(connection, keys.data());
    const auto byte = static_cast<unsigned char>(keys.at(static_cast<std::size_t>(keycode >> 3)));
    return (byte & (1U << (keycode & 7))) != 0;
}

std::optional<std::string> keyLabel(int scancode)
{
    auto* connection = display();
    const int keycode = scancode + kKeycodeOffset;
    if (connection == nullptr || keycode > 255)
        return std::nullopt;
    const KeySym keysym = XkbKeycodeToKeysym(connection, static_cast<KeyCode>(keycode), 0, 0);
    char32_t character = 0;
    if (keysym > 0x20 && keysym < 0x100 && keysym != 0x7F && keysym != 0xA0)
        character = static_cast<char32_t>(keysym);
    else if ((keysym & 0xFF000000) == kUnicodeKeysym)
        character = static_cast<char32_t>(keysym & 0x00FFFFFF);
    if (character == 0 || character >= 0x10000)
        return std::nullopt;
    return toUtf8(character);
}

} // namespace drumprog::app::native
