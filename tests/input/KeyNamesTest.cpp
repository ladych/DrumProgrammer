#include "input/KeyNames.h"

#include "input/Scancode.h"

#include <gtest/gtest.h>

namespace drumprog::input
{
namespace
{

TEST(UsKeyNamesTest, FIN02_NamesKeysAfterTheUsLayout)
{
    const UsKeyNames names;
    EXPECT_EQ(names.nameOf(scancode::kA), "A");
    EXPECT_EQ(names.nameOf(scancode::kZ), "Z");
    EXPECT_EQ(names.nameOf(scancode::kDigit0), "0");
    EXPECT_EQ(names.nameOf(scancode::kSpace), "Leertaste");
}

TEST(UsKeyNamesTest, FIN02_UnknownKeysGetTheirNumber)
{
    const UsKeyNames names;
    EXPECT_EQ(names.nameOf(0), "Taste 0");
    EXPECT_EQ(names.nameOf(0x3A), "Taste 58");
}

} // namespace
} // namespace drumprog::input
