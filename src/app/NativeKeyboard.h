#pragma once

#include <optional>
#include <string>

/// Platform access to the physical keyboard (humble object, E-03): JUCE key presses carry layout
/// dependent key codes, the drum mapping needs physical keys (scancodes, Pflichtenheft 6.3). Each
/// platform has its own implementation file (Q-13); the header is free of JUCE and system headers.
namespace drumprog::app::native
{

/// Physical key of a juce::KeyPress key code in the current keyboard layout; nullopt if unknown.
[[nodiscard]] std::optional<int> scancodeForKeyCode(int juceKeyCode);

/// Whether the physical key is held down right now.
[[nodiscard]] bool isScancodeDown(int scancode);

/// Character of the physical key in the current keyboard layout (UTF-8, as the layout produces it
/// without Shift); nullopt if unknown.
[[nodiscard]] std::optional<std::string> keyLabel(int scancode);

} // namespace drumprog::app::native
