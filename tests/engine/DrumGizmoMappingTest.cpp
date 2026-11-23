#include "engine/DrumGizmoMapping.h"

#include "engine/KitDescription.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <string>

namespace drumprog::engine
{
namespace
{

DrumGizmoKit kitWith(std::initializer_list<std::string> names)
{
    DrumGizmoKit kit;
    for (const auto& name : names)
        kit.instruments.push_back({.name = name, .group = "", .samples = {}});
    return kit;
}

std::string instrumentFor(const std::map<int, DrumGizmoSlotMapping>& mapping, int gmNote)
{
    return mapping.at(gmNote).instrument;
}

TEST(DrumGizmoMapping, FDG07_EveryGmSlotGetsAnEntry)
{
    const auto mapping = mapToGmSlots(kitWith({}));

    EXPECT_EQ(mapping.size(), makeGmDefaultKit().size());
    EXPECT_EQ(instrumentFor(mapping, 36), "");
    EXPECT_EQ(mapping.at(42).chokeGroup, kHiHatChokeGroup);
    EXPECT_EQ(mapping.at(36).chokeGroup, 0);
}

TEST(DrumGizmoMapping, FDG07_TheKitsMidiMapComesFirst)
{
    auto kit = kitWith({"Kdrum", "KickSub", "Snare"});
    kit.midiMap = {{36, "KickSub"}, {38, "Missing"}};

    const auto mapping = mapToGmSlots(kit);

    EXPECT_EQ(instrumentFor(mapping, 36), "KickSub");
    // A MIDI map entry for an instrument the kit does not have falls back to the names.
    EXPECT_EQ(instrumentFor(mapping, 38), "Snare");
    EXPECT_EQ(instrumentFor(mapping, 35), "Kdrum");
}

TEST(DrumGizmoMapping, FDG07_KickAndSnareAreFoundByName)
{
    const auto mapping = mapToGmSlots(kitWith({"Bass Drum", "Snare_rim", "Snare", "SideStick"}));

    EXPECT_EQ(instrumentFor(mapping, 36), "Bass Drum");
    EXPECT_EQ(instrumentFor(mapping, 38), "Snare");
    EXPECT_EQ(instrumentFor(mapping, 40), "Snare");
    EXPECT_EQ(instrumentFor(mapping, 37), "Snare_rim");
}

TEST(DrumGizmoMapping, FDG07_SnareVariantsAndSideStickFallBack)
{
    const auto mapping = mapToGmSlots(kitWith({"KICK", "Snare_rimshot", "Stick"}));

    EXPECT_EQ(instrumentFor(mapping, 36), "KICK");
    EXPECT_EQ(instrumentFor(mapping, 38), "Snare_rimshot");
    EXPECT_EQ(instrumentFor(mapping, 37), "Snare_rimshot");
    EXPECT_EQ(mapToGmSlots(kitWith({"Rim"})).at(37).instrument, "Rim");
}

TEST(DrumGizmoMapping, FDG07_HiHatPositionsUseTheirVariantOrThePlainHiHat)
{
    const auto full = mapToGmSlots(kitWith({"HihatOpen", "Hihat_Foot", "HiHat closed", "Hihat"}));
    EXPECT_EQ(instrumentFor(full, 42), "HiHat closed");
    EXPECT_EQ(instrumentFor(full, 44), "Hihat_Foot");
    EXPECT_EQ(instrumentFor(full, 46), "HihatOpen");

    const auto plain = mapToGmSlots(kitWith({"HihatOpen", "Hi-Hat"}));
    EXPECT_EQ(instrumentFor(plain, 42), "Hi-Hat");
    EXPECT_EQ(instrumentFor(plain, 44), "Hi-Hat");

    const auto closedOnly = mapToGmSlots(kitWith({"hh_tight"}));
    EXPECT_EQ(instrumentFor(closedOnly, 44), "hh_tight");
    EXPECT_EQ(instrumentFor(closedOnly, 46), "hh_tight");

    const auto openOnly = mapToGmSlots(kitWith({"Hihat_pedal", "Hihat_open"}));
    EXPECT_EQ(instrumentFor(openOnly, 42), "Hihat_pedal");
}

TEST(DrumGizmoMapping, FDG07_CymbalsAndPercussionAreFoundByName)
{
    const auto mapping = mapToGmSlots(kitWith({"Crash1",
                                               "Crash2",
                                               "Ride",
                                               "RideBell",
                                               "Ride2",
                                               "China",
                                               "Tambourine",
                                               "Cowbell",
                                               "Vibraslap",
                                               "Clap"}));

    EXPECT_EQ(instrumentFor(mapping, 49), "Crash1");
    EXPECT_EQ(instrumentFor(mapping, 57), "Crash2");
    EXPECT_EQ(instrumentFor(mapping, 55), "Crash1");
    EXPECT_EQ(instrumentFor(mapping, 51), "Ride");
    EXPECT_EQ(instrumentFor(mapping, 59), "Ride2");
    EXPECT_EQ(instrumentFor(mapping, 53), "RideBell");
    EXPECT_EQ(instrumentFor(mapping, 52), "China");
    EXPECT_EQ(instrumentFor(mapping, 54), "Tambourine");
    EXPECT_EQ(instrumentFor(mapping, 56), "Cowbell");
    EXPECT_EQ(instrumentFor(mapping, 58), "Vibraslap");
    EXPECT_EQ(instrumentFor(mapping, 39), "Clap");
}

TEST(DrumGizmoMapping, FDG07_SingleCymbalsServeEveryVariant)
{
    const auto mapping = mapToGmSlots(kitWith({"Crash", "Ride", "Splash"}));

    EXPECT_EQ(instrumentFor(mapping, 57), "Crash");
    EXPECT_EQ(instrumentFor(mapping, 59), "Ride");
    EXPECT_EQ(instrumentFor(mapping, 53), "Ride");
    EXPECT_EQ(instrumentFor(mapping, 55), "Splash");
}

TEST(DrumGizmoMapping, FDG07_FourTomsSpreadOverTheSixTomNotes)
{
    const auto mapping = mapToGmSlots(kitWith({"FloorTom", "Tom2", "Tom1", "Tom3"}));

    EXPECT_EQ(instrumentFor(mapping, 50), "Tom1");
    EXPECT_EQ(instrumentFor(mapping, 48), "Tom1");
    EXPECT_EQ(instrumentFor(mapping, 47), "Tom2");
    EXPECT_EQ(instrumentFor(mapping, 45), "Tom2");
    EXPECT_EQ(instrumentFor(mapping, 43), "Tom3");
    EXPECT_EQ(instrumentFor(mapping, 41), "FloorTom");
}

TEST(DrumGizmoMapping, FDG07_EveryNumberOfTomsFillsAllTomNotes)
{
    const std::vector<std::string> toms{"Tom1", "Tom2", "Tom3", "Tom4", "Tom5", "Tom6", "Tom7"};
    for (std::size_t count = 1; count <= toms.size(); ++count)
    {
        DrumGizmoKit kit;
        for (std::size_t index = 0; index < count; ++index)
            kit.instruments.push_back({.name = toms[index], .group = "", .samples = {}});

        const auto mapping = mapToGmSlots(kit);

        for (const int note : {50, 48, 47, 45, 43, 41})
            EXPECT_FALSE(instrumentFor(mapping, note).empty()) << count << " toms, note " << note;
        EXPECT_EQ(instrumentFor(mapping, 50), "Tom1");
        EXPECT_EQ(instrumentFor(mapping, 41), toms[std::min<std::size_t>(count, 6) - 1]) << count;
    }
}

TEST(DrumGizmoMapping, FDG07_EverySpellingOfKickHiHatStickAndFloorTomIsFound)
{
    EXPECT_EQ(mapToGmSlots(kitWith({"BassDrum"})).at(36).instrument, "BassDrum");
    EXPECT_EQ(mapToGmSlots(kitWith({"Stick"})).at(37).instrument, "Stick");
    EXPECT_EQ(mapToGmSlots(kitWith({"Hat"})).at(42).instrument, "Hat");
    const auto toms = mapToGmSlots(kitWith({"FTom", "Tom"}));
    EXPECT_EQ(toms.at(50).instrument, "Tom");
    EXPECT_EQ(toms.at(41).instrument, "FTom");
}

TEST(DrumGizmoMapping, FDG07_SnareBottomIsNoTom)
{
    const auto mapping = mapToGmSlots(kitWith({"Snare_bottom"}));

    EXPECT_EQ(instrumentFor(mapping, 50), "");
    EXPECT_EQ(instrumentFor(mapping, 38), "Snare_bottom");
}

TEST(DrumGizmoMapping, FDG08_AnHhGroupIsTheHiHatChokeGroup)
{
    DrumGizmoKit kit;
    kit.instruments = {{.name = "Crash", .group = "Cymbals", .samples = {}},
                       {.name = "Hihat", .group = "HH", .samples = {}}};

    const auto mapping = mapToGmSlots(kit);

    EXPECT_EQ(mapping.at(49).chokeGroup, kHiHatChokeGroup + 1);
    EXPECT_EQ(mapping.at(46).chokeGroup, kHiHatChokeGroup);
}

TEST(DrumGizmoMapping, FDG08_InstrumentGroupsBecomeChokeGroups)
{
    DrumGizmoKit kit;
    kit.instruments = {{.name = "Hihat", .group = "hihat", .samples = {}},
                       {.name = "Crash", .group = "cymbals", .samples = {}},
                       {.name = "China", .group = "cymbals", .samples = {}},
                       {.name = "Kick", .group = "", .samples = {}}};

    const auto mapping = mapToGmSlots(kit);

    EXPECT_EQ(mapping.at(42).chokeGroup, kHiHatChokeGroup);
    EXPECT_EQ(mapping.at(49).chokeGroup, kHiHatChokeGroup + 1);
    EXPECT_EQ(mapping.at(52).chokeGroup, kHiHatChokeGroup + 1);
    EXPECT_EQ(mapping.at(36).chokeGroup, 0);
}

} // namespace
} // namespace drumprog::engine
