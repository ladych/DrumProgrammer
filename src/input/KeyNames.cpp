#include "input/KeyNames.h"

#include <array>
#include <cstddef>
#include <string_view>

namespace drumprog::input
{
namespace
{

/// US labels of the set-1 scancodes 0x00..0x39 (main block up to the space bar).
constexpr std::array<std::string_view, 0x3A> kUsNames{
    "",  "Esc", "1",         "2",   "3",     "4",    "5",        "6",     "7",   "8",         "9", "0",
    "-", "=",   "Rücktaste", "Tab", "Q",     "W",    "E",        "R",     "T",   "Y",         "U", "I",
    "O", "P",   "[",         "]",   "Enter", "Strg", "A",        "S",     "D",   "F",         "G", "H",
    "J", "K",   "L",         ";",   "'",     "`",    "Umschalt", "\\",    "Z",   "X",         "C", "V",
    "B", "N",   "M",         ",",   ".",     "/",    "Umschalt", "Num *", "Alt", "Leertaste",
};

} // namespace

std::string UsKeyNames::nameOf(int scancode) const
{
    if (scancode > 0 && scancode < static_cast<int>(kUsNames.size()))
        return std::string{kUsNames.at(static_cast<std::size_t>(scancode))};
    return "Taste " + std::to_string(scancode);
}

} // namespace drumprog::input
