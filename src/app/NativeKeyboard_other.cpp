#include "app/NativeKeyboard.h"

// Platforms without physical key access (only Linux and Windows are supported, Pflichtenheft
// chapter 2): letters and digits map to their US-layout scancodes, nothing else is known.

#include <array>

namespace drumprog::app::native
{
namespace
{

constexpr std::array<int, 26> kLetterScancodes{30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50,
                                               49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44};

} // namespace

std::optional<int> scancodeForKeyCode(int juceKeyCode)
{
    if (juceKeyCode >= 'a' && juceKeyCode <= 'z')
        juceKeyCode -= 'a' - 'A';
    if (juceKeyCode >= 'A' && juceKeyCode <= 'Z')
        return kLetterScancodes.at(static_cast<std::size_t>(juceKeyCode - 'A'));
    if (juceKeyCode >= '1' && juceKeyCode <= '9')
        return juceKeyCode - '1' + 2;
    if (juceKeyCode == '0')
        return 11;
    return std::nullopt;
}

bool isScancodeDown(int /*scancode*/)
{
    return false;
}

std::optional<std::string> keyLabel(int /*scancode*/)
{
    return std::nullopt;
}

} // namespace drumprog::app::native
