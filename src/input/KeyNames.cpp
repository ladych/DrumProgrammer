#include "input/KeyNames.h"

#include "i18n/Translate.h"

#include <array>
#include <cstddef>

namespace drumprog::input
{
namespace
{

/// US labels of the set-1 scancodes 0x00..0x39 (main block up to the space bar); the names of the keys
/// without a character are translated.
constexpr std::array<const char*, 0x3A> kUsNames{
    "",
    "Esc",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "-",
    "=",
    i18n::marked("Backspace"),
    "Tab",
    "Q",
    "W",
    "E",
    "R",
    "T",
    "Y",
    "U",
    "I",
    "O",
    "P",
    "[",
    "]",
    "Enter",
    i18n::marked("Ctrl"),
    "A",
    "S",
    "D",
    "F",
    "G",
    "H",
    "J",
    "K",
    "L",
    ";",
    "'",
    "`",
    i18n::marked("Shift"),
    "\\",
    "Z",
    "X",
    "C",
    "V",
    "B",
    "N",
    "M",
    ",",
    ".",
    "/",
    i18n::marked("Shift"),
    "Num *",
    "Alt",
    i18n::marked("Space"),
};

} // namespace

std::string UsKeyNames::nameOf(int scancode) const
{
    if (scancode > 0 && scancode < static_cast<int>(kUsNames.size()))
        return i18n::tr(kUsNames.at(static_cast<std::size_t>(scancode)));
    return i18n::tr("Key %1", std::to_string(scancode));
}

} // namespace drumprog::input
