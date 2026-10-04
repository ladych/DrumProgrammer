#pragma once

namespace drumprog::input
{

/// Physical key positions as PC/AT set-1 scancodes, so the mapping is the same on QWERTY and QWERTZ
/// keyboards (Pflichtenheft 6.3, K5). For the main key block they equal the Linux evdev key codes
/// (X11 keycode minus 8) and the scancodes Windows reports. Extended keys carry kExtended.
namespace scancode
{
inline constexpr int kExtended = 0x100;

inline constexpr int kEscape = 1;
inline constexpr int kDigit1 = 2;
inline constexpr int kDigit7 = 8;
inline constexpr int kDigit8 = 9;
inline constexpr int kDigit9 = 10;
inline constexpr int kDigit0 = 11;
inline constexpr int kQ = 16;
inline constexpr int kW = 17;
inline constexpr int kE = 18;
inline constexpr int kR = 19;
inline constexpr int kT = 20;
inline constexpr int kY = 21; ///< Z on a German keyboard
inline constexpr int kU = 22;
inline constexpr int kI = 23;
inline constexpr int kLeftControl = 29;
inline constexpr int kA = 30;
inline constexpr int kS = 31;
inline constexpr int kD = 32;
inline constexpr int kF = 33;
inline constexpr int kLeftShift = 42;
inline constexpr int kZ = 44; ///< Y on a German keyboard
inline constexpr int kX = 45;
inline constexpr int kRightShift = 54;
inline constexpr int kLeftAlt = 56;
inline constexpr int kSpace = 57;
inline constexpr int kCapsLock = 58;
inline constexpr int kRightControl = kExtended | kLeftControl;
inline constexpr int kRightAlt = kExtended | kLeftAlt;
inline constexpr int kLeftMeta = kExtended | 0x5B;
inline constexpr int kRightMeta = kExtended | 0x5C;
inline constexpr int kMax = 0x1FF;
} // namespace scancode

/// Shift, Ctrl, Alt, Meta and Caps Lock are never drum keys: Shift is the accent (F-IN-01), Ctrl the
/// menu shortcuts (6.4).
[[nodiscard]] bool isModifierKey(int scancode) noexcept;

/// 1..kMax.
[[nodiscard]] bool isValidScancode(int scancode) noexcept;

} // namespace drumprog::input
