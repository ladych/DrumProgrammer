#include "input/Keymap.h"

#include "input/Scancode.h"

#include <gtest/gtest.h>

namespace drumprog::input
{
namespace
{

TEST(KeymapTest, FIN01_DefaultsFollowTheGuiDraft)
{
    const auto keymap = Keymap::defaults();

    EXPECT_EQ(keymap.keyFor(36), scancode::kA);
    EXPECT_EQ(keymap.keyFor(38), scancode::kS);
    EXPECT_EQ(keymap.keyFor(42), scancode::kW);
    EXPECT_EQ(keymap.keyFor(49), scancode::kDigit0);
    EXPECT_EQ(keymap.keyFor(57), scancode::kDigit9);
    EXPECT_FALSE(keymap.keyFor(35).has_value());
    EXPECT_TRUE(keymap.duplicates().empty());
}

TEST(KeymapTest, K5_HighTomIsOnThePhysicalKeyLeftOfX)
{
    // "Z" in the draft is the QWERTY position, labelled "Y" on a German keyboard.
    EXPECT_EQ(Keymap::defaults().keyFor(50), scancode::kZ);
}

TEST(KeymapTest, FIN01_FindsTheNotesOfAKey)
{
    const auto keymap = Keymap::defaults();
    EXPECT_EQ(keymap.notesFor(scancode::kS), (std::vector<int>{38}));
    EXPECT_TRUE(keymap.notesFor(scancode::kY).empty());
}

TEST(KeymapTest, FIN02_AssignReplacesTheKeyOfANote)
{
    auto keymap = Keymap::defaults();
    keymap.assign(38, scancode::kY);
    EXPECT_EQ(keymap.keyFor(38), scancode::kY);
    EXPECT_TRUE(keymap.notesFor(scancode::kS).empty());
}

TEST(KeymapTest, FIN02_AssignIgnoresInvalidNotesAndKeys)
{
    Keymap keymap;
    keymap.assign(128, scancode::kA);
    keymap.assign(-1, scancode::kA);
    keymap.assign(36, 0);
    keymap.assign(36, scancode::kMax + 1);
    EXPECT_TRUE(keymap.notesFor(scancode::kA).empty());
    EXPECT_FALSE(keymap.keyFor(36).has_value());
    keymap.assign(0, scancode::kA);
    keymap.assign(127, scancode::kA);
    EXPECT_EQ(keymap.notesFor(scancode::kA), (std::vector<int>{0, 127}));
}

TEST(KeymapTest, FIN02_ClearRemovesTheKey)
{
    auto keymap = Keymap::defaults();
    keymap.clear(36);
    EXPECT_FALSE(keymap.keyFor(36).has_value());
}

TEST(KeymapTest, FIN02_ReportsKeysGivenToSeveralNotes)
{
    auto keymap = Keymap::defaults();
    keymap.assign(46, scancode::kW);
    keymap.assign(35, scancode::kW);

    const auto duplicates = keymap.duplicates();

    ASSERT_EQ(duplicates.size(), 1U);
    EXPECT_EQ(duplicates.at(scancode::kW), (std::vector<int>{35, 42, 46}));
    EXPECT_EQ(keymap.notesFor(scancode::kW), (std::vector<int>{35, 42, 46}));
}

TEST(KeymapTest, FIN02_TextRoundTrip)
{
    auto keymap = Keymap::defaults();
    keymap.assign(38, scancode::kY);
    keymap.clear(36);

    const auto restored = Keymap::fromText(keymap.toText());

    EXPECT_EQ(restored.keyFor(38), scancode::kY);
    EXPECT_FALSE(restored.keyFor(36).has_value());
    EXPECT_EQ(restored.keyFor(57), scancode::kDigit9);
}

TEST(KeymapTest, FIN02_NoSavedTextGivesTheDefaults)
{
    EXPECT_EQ(Keymap::fromText(std::nullopt).keyFor(36), scancode::kA);
}

TEST(KeymapTest, FIN02_SavedEmptyMappingStaysEmpty)
{
    EXPECT_FALSE(Keymap::fromText(Keymap{}.toText()).keyFor(36).has_value());
}

TEST(KeymapTest, FIN02_SkipsMalformedLines)
{
    const auto keymap = Keymap::fromText("# comment\n36=30\nfoo\n38:31\n40=\n200=30\n42=17 trailing\n");

    EXPECT_EQ(keymap.keyFor(36), scancode::kA);
    EXPECT_FALSE(keymap.keyFor(38).has_value());
    EXPECT_FALSE(keymap.keyFor(40).has_value());
    EXPECT_EQ(keymap.notesFor(scancode::kA), (std::vector<int>{36}));
    EXPECT_EQ(keymap.keyFor(42), scancode::kW);
}

} // namespace
} // namespace drumprog::input
