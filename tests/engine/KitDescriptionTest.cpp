#include "engine/KitDescription.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace drumprog::engine
{
namespace
{

const KitSlotDescription& slotWithNote(const KitDescription& kit, int note)
{
    return *std::find_if(kit.begin(), kit.end(), [note](const auto& slot) { return slot.gmNote == note; });
}

TEST(KitDescriptionTest, FSE04_DefaultKitCoversGmNotes35To59)
{
    const auto kit = makeGmDefaultKit();
    ASSERT_EQ(kit.size(), 25U);
    for (size_t i = 0; i < kit.size(); ++i)
    {
        EXPECT_EQ(kit[i].gmNote, 35 + static_cast<int>(i));
        EXPECT_EQ(kit[i].midiNote, kit[i].gmNote);
        EXPECT_FALSE(kit[i].name.empty());
        EXPECT_TRUE(kit[i].sampleFile.empty());
        EXPECT_FLOAT_EQ(kit[i].gain, 1.0F);
        EXPECT_EQ(kit[i].pitchSemitones, 0);
    }
}

TEST(KitDescriptionTest, FSE04_DefaultKitUsesGmNames)
{
    const auto kit = makeGmDefaultKit();
    EXPECT_EQ(slotWithNote(kit, 36).name, "Bass Drum 1");
    EXPECT_EQ(slotWithNote(kit, 38).name, "Acoustic Snare");
    EXPECT_EQ(slotWithNote(kit, 42).name, "Closed Hi-Hat");
    EXPECT_EQ(slotWithNote(kit, 59).name, "Ride Cymbal 2");
}

TEST(KitDescriptionTest, FSE04_CoreSlotsAreTheMainDrumKitPieces)
{
    const auto kit = makeGmDefaultKit();
    const auto coreCount = std::count_if(kit.begin(), kit.end(), [](const auto& slot) { return slot.coreSlot; });
    EXPECT_GE(coreCount, 16);
    EXPECT_LE(coreCount, 20);
    EXPECT_TRUE(slotWithNote(kit, 36).coreSlot);
    EXPECT_TRUE(slotWithNote(kit, 46).coreSlot);
    EXPECT_FALSE(slotWithNote(kit, 58).coreSlot);
}

TEST(KitDescriptionTest, FSE08_HiHatsShareTheChokeGroup)
{
    const auto kit = makeGmDefaultKit();
    for (const auto& slot : kit)
    {
        const bool hiHat = slot.gmNote == 42 || slot.gmNote == 44 || slot.gmNote == 46;
        EXPECT_EQ(slot.chokeGroup, hiHat ? kHiHatChokeGroup : 0) << slot.gmNote;
    }
}

TEST(KitDescriptionTest, FSE05_DetectsOverriddenMidiNote)
{
    KitSlotDescription slot{.gmNote = 38, .midiNote = 38};
    EXPECT_TRUE(slot.usesGmNote());
    slot.midiNote = 40;
    EXPECT_FALSE(slot.usesGmNote());
}

} // namespace
} // namespace drumprog::engine
