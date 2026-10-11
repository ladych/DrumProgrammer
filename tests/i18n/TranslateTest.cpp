#include "i18n/Translate.h"

#include "i18n/TestLanguage.h"

#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace drumprog::i18n
{
namespace
{

TEST(Translate, FUI01_ShowsTheTextInTheCurrentLanguage)
{
    EXPECT_EQ(tr("Delete notes"), "Noten löschen");
    EXPECT_EQ(trJuce("Delete notes"), juce::String::fromUTF8("Noten löschen"));

    const test::ScopedEnglish english;
    EXPECT_EQ(tr("Delete notes"), "Delete notes");
    EXPECT_EQ(trJuce("Delete notes"), "Delete notes");
}

TEST(Translate, FUI03_KeepsTextsWithoutTranslationEnglish)
{
    EXPECT_EQ(tr("No such text"), "No such text");
}

TEST(Translate, FUI01_ReplacesPlaceholdersAfterTranslating)
{
    EXPECT_EQ(tr("Key %1 is assigned twice: %2", "Q", "38 Snare"), "Taste Q ist doppelt belegt: 38 Snare");
    EXPECT_EQ(trJuce("%1 notes", "3"), "3 Noten");
}

TEST(Translate, FUI01_DecimalSeparatorFollowsTheLanguage)
{
    EXPECT_EQ(decimalSeparator(), ",");
    const test::ScopedEnglish english;
    EXPECT_EQ(decimalSeparator(), ".");
}

TEST(Translate, FUI01_SubstitutesEachPlaceholderOnce)
{
    EXPECT_EQ(substitute("%2 before %1", std::array<std::string_view, 2>{"a", "b"}), "b before a");
    // An argument's own "%2" is not replaced again.
    EXPECT_EQ(substitute("%1 and %2", std::array<std::string_view, 2>{"%2", "x"}), "%2 and x");
    EXPECT_EQ(substitute("%1 %1", std::array<std::string_view, 1>{"a"}), "a a");
}

TEST(Translate, FUI01_LeavesOtherPercentSignsAsTheyAre)
{
    EXPECT_EQ(substitute("100% %0 %a %3 %", std::array<std::string_view, 2>{"a", "b"}), "100% %0 %a %3 %");
    EXPECT_EQ(substitute("no placeholders", {}), "no placeholders");
}

TEST(Translate, FUI01_MarkedTextIsTheEnglishText)
{
    static_assert(marked("Recording")[0] == 'R');
    EXPECT_STREQ(marked("Recording"), "Recording");
}

} // namespace
} // namespace drumprog::i18n
