#include "input/KeyboardInput.h"

#include "input/FakeNoteSink.h"
#include "input/Scancode.h"

#include <gtest/gtest.h>

#include <utility>

namespace drumprog::input
{
namespace
{

using Hit = std::pair<int, int>;

class KeyboardInputTest : public ::testing::Test
{
protected:
    Keymap keymap = Keymap::defaults();
    FakeNoteSink sink;
    InputActivity activity;
    KeyboardInput keyboard{keymap, sink, activity};
};

TEST_F(KeyboardInputTest, FIN01_MappedKeyTriggersItsNoteWithVelocity100)
{
    EXPECT_TRUE(keyboard.keyDown(scancode::kS, {}, false));
    EXPECT_EQ(sink.hits, (std::vector<Hit>{{38, 100}}));
}

TEST_F(KeyboardInputTest, FIN01_ShiftPlaysWithVelocity127)
{
    keyboard.keyDown(scancode::kA, {.shift = true}, false);
    EXPECT_EQ(sink.hits, (std::vector<Hit>{{36, 127}}));
}

TEST_F(KeyboardInputTest, FIN01_UnmappedKeyIsLeftToOthers)
{
    EXPECT_FALSE(keyboard.keyDown(scancode::kY, {}, false));
    EXPECT_TRUE(sink.hits.empty());
    EXPECT_EQ(activity.keyboardCount(), 0U);
}

TEST_F(KeyboardInputTest, FIN06_NoTriggerWhileATextFieldHasTheFocus)
{
    EXPECT_FALSE(keyboard.keyDown(scancode::kS, {}, true));
    EXPECT_TRUE(sink.hits.empty());
}

TEST_F(KeyboardInputTest, FIN01_MenuShortcutsAreNoDrumHits)
{
    EXPECT_FALSE(keyboard.keyDown(scancode::kS, {.commandOrAlt = true}, false));
    EXPECT_TRUE(sink.hits.empty());
}

TEST_F(KeyboardInputTest, FIN01_AutoRepeatOfAHeldKeyDoesNotTriggerAgain)
{
    keyboard.keyDown(scancode::kS, {}, false);
    EXPECT_TRUE(keyboard.keyDown(scancode::kS, {}, false));
    EXPECT_EQ(sink.hits.size(), 1U);
}

TEST_F(KeyboardInputTest, FIN01_ReleasedKeyTriggersAgain)
{
    keyboard.keyDown(scancode::kS, {}, false);
    keyboard.keyDown(scancode::kA, {}, false);
    keyboard.releaseKeysNotDown([](int key) { return key == scancode::kA; });

    EXPECT_EQ(keyboard.heldKeys(), (std::set<int>{scancode::kA}));
    keyboard.keyDown(scancode::kS, {}, false);
    keyboard.keyDown(scancode::kA, {}, false);
    EXPECT_EQ(sink.hits, (std::vector<Hit>{{38, 100}, {36, 100}, {38, 100}}));
}

TEST_F(KeyboardInputTest, FIN02_KeyGivenToSeveralNotesTriggersAllOfThem)
{
    keymap.assign(46, scancode::kW);
    keyboard.keyDown(scancode::kW, {}, false);
    EXPECT_EQ(sink.hits, (std::vector<Hit>{{42, 100}, {46, 100}}));
}

TEST_F(KeyboardInputTest, FIN02_ChangedMappingIsUsedRightAway)
{
    keymap.assign(38, scancode::kY);
    keyboard.keyDown(scancode::kY, {}, false);
    EXPECT_EQ(sink.hits, (std::vector<Hit>{{38, 100}}));
}

TEST_F(KeyboardInputTest, FIN05_EveryHitSignalsTheKeyboardLed)
{
    keyboard.keyDown(scancode::kS, {}, false);
    keyboard.keyDown(scancode::kA, {}, false);
    EXPECT_EQ(activity.keyboardCount(), 2U);
    EXPECT_EQ(activity.midiCount(), 0U);
}

} // namespace
} // namespace drumprog::input
