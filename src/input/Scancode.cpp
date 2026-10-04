#include "input/Scancode.h"

namespace drumprog::input
{

bool isModifierKey(int scancode) noexcept
{
    switch (scancode)
    {
    case scancode::kLeftControl:
    case scancode::kRightControl:
    case scancode::kLeftShift:
    case scancode::kRightShift:
    case scancode::kLeftAlt:
    case scancode::kRightAlt:
    case scancode::kLeftMeta:
    case scancode::kRightMeta:
    case scancode::kCapsLock:
        return true;
    default:
        return false;
    }
}

bool isValidScancode(int scancode) noexcept
{
    return scancode > 0 && scancode <= scancode::kMax;
}

} // namespace drumprog::input
