#include "engine/EngineKit.h"

#include <gtest/gtest.h>

namespace drumprog::engine
{
namespace
{

TEST(EngineKitTest, FSE05_FindsSlotByMidiNote)
{
    const EngineKit kit(
        {EngineSlot{.slotIndex = 0, .midiNote = 36}, EngineSlot{.slotIndex = 1, .midiNote = 40}});
    ASSERT_EQ(kit.slots().size(), 2U);
    ASSERT_NE(kit.slotForNote(36), nullptr);
    EXPECT_EQ(kit.slotForNote(36)->slotIndex, 0);
    ASSERT_NE(kit.slotForNote(40), nullptr);
    EXPECT_EQ(kit.slotForNote(40)->slotIndex, 1);
    EXPECT_EQ(kit.slotForNote(38), nullptr);
}

TEST(EngineKitTest, FSE05_ReturnsNothingOutsideMidiRange)
{
    const EngineKit kit(
        {EngineSlot{.slotIndex = 0, .midiNote = 0}, EngineSlot{.slotIndex = 1, .midiNote = 127}});
    EXPECT_NE(kit.slotForNote(0), nullptr);
    EXPECT_NE(kit.slotForNote(127), nullptr);
    EXPECT_EQ(kit.slotForNote(-1), nullptr);
    EXPECT_EQ(kit.slotForNote(128), nullptr);
}

TEST(EngineKitTest, FSE05_IgnoresSlotsWithInvalidNoteAndKeepsFirstDuplicate)
{
    const EngineKit kit({EngineSlot{.slotIndex = 0, .midiNote = -1},
                         EngineSlot{.slotIndex = 1, .midiNote = 200},
                         EngineSlot{.slotIndex = 2, .midiNote = 38},
                         EngineSlot{.slotIndex = 3, .midiNote = 38}});
    ASSERT_NE(kit.slotForNote(38), nullptr);
    EXPECT_EQ(kit.slotForNote(38)->slotIndex, 2);
}

} // namespace
} // namespace drumprog::engine
