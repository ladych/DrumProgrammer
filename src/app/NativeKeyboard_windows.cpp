#include "app/NativeKeyboard.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <array>
#include <cstddef>

namespace drumprog::app::native
{
namespace
{

/// juce::KeyPress marks non-character keys with this bit on Windows (juce_Windowing_windows.cpp).
constexpr int kJuceExtendedKey = 0x10000;
/// Our scancodes mark extended keys with 0x100, Windows with the prefix byte 0xE0.
constexpr int kExtendedScancode = 0x100;
constexpr UINT kWindowsExtendedPrefix = 0xE000;

std::optional<UINT> virtualKeyForKeyCode(int keyCode)
{
    if ((keyCode & kJuceExtendedKey) != 0 || keyCode < 0x20)
        return static_cast<UINT>(keyCode & 0xffff);
    const SHORT result = VkKeyScanW(static_cast<WCHAR>(keyCode));
    if (result == -1)
        return std::nullopt;
    return static_cast<UINT>(LOBYTE(result));
}

} // namespace

std::optional<int> scancodeForKeyCode(int juceKeyCode)
{
    const auto virtualKey = virtualKeyForKeyCode(juceKeyCode);
    if (!virtualKey)
        return std::nullopt;
    const UINT scancode = MapVirtualKeyW(*virtualKey, MAPVK_VK_TO_VSC_EX);
    if (scancode == 0)
        return std::nullopt;
    const bool extended = (scancode & 0xFF00) == kWindowsExtendedPrefix;
    return static_cast<int>(scancode & 0xFF) | (extended ? kExtendedScancode : 0);
}

bool isScancodeDown(int scancode)
{
    const bool extended = (scancode & kExtendedScancode) != 0;
    const UINT windowsScancode = static_cast<UINT>(scancode & 0xFF) | (extended ? kWindowsExtendedPrefix : 0);
    const UINT virtualKey = MapVirtualKeyW(windowsScancode, MAPVK_VSC_TO_VK_EX);
    return virtualKey != 0 && (GetAsyncKeyState(static_cast<int>(virtualKey)) & 0x8000) != 0;
}

std::optional<std::string> keyLabel(int scancode)
{
    std::array<wchar_t, 64> name{};
    const int length =
        GetKeyNameTextW(static_cast<LONG>(scancode) << 16, name.data(), static_cast<int>(name.size()));
    if (length <= 0)
        return std::nullopt;
    const int size = WideCharToMultiByte(CP_UTF8, 0, name.data(), length, nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, name.data(), length, utf8.data(), size, nullptr, nullptr);
    return utf8;
}

} // namespace drumprog::app::native
