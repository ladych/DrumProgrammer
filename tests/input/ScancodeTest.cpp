#include "input/Scancode.h"

#include <gtest/gtest.h>

namespace drumprog::input
{
namespace
{

TEST(ScancodeTest, FIN01_ModifierKeysAreNeverDrumKeys)
{
    for (const int key : {scancode::kLeftShift,
                          scancode::kRightShift,
                          scancode::kLeftControl,
                          scancode::kRightControl,
                          scancode::kLeftAlt,
                          scancode::kRightAlt,
                          scancode::kLeftMeta,
                          scancode::kRightMeta,
                          scancode::kCapsLock})
        EXPECT_TRUE(isModifierKey(key)) << key;
    EXPECT_FALSE(isModifierKey(scancode::kA));
    EXPECT_FALSE(isModifierKey(scancode::kSpace));
}

TEST(ScancodeTest, FIN02_ValidScancodesAreOneToMax)
{
    EXPECT_FALSE(isValidScancode(0));
    EXPECT_TRUE(isValidScancode(1));
    EXPECT_TRUE(isValidScancode(scancode::kMax));
    EXPECT_FALSE(isValidScancode(scancode::kMax + 1));
}

} // namespace
} // namespace drumprog::input
