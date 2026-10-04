#include "engine/KitDescription.h"

#include <array>
#include <string_view>

namespace drumprog::engine
{
namespace
{

struct GmDrum
{
    int note;
    std::string_view name;
    bool core;
};

constexpr std::array<GmDrum, 25> kGmDrums{{
    {35, "Acoustic Bass Drum", false},
    {36, "Bass Drum 1", true},
    {37, "Side Stick", true},
    {38, "Acoustic Snare", true},
    {39, "Hand Clap", true},
    {40, "Electric Snare", true},
    {41, "Low Floor Tom", true},
    {42, "Closed Hi-Hat", true},
    {43, "High Floor Tom", true},
    {44, "Pedal Hi-Hat", true},
    {45, "Low Tom", true},
    {46, "Open Hi-Hat", true},
    {47, "Low-Mid Tom", true},
    {48, "Hi-Mid Tom", true},
    {49, "Crash Cymbal 1", true},
    {50, "High Tom", true},
    {51, "Ride Cymbal 1", true},
    {52, "Chinese Cymbal", false},
    {53, "Ride Bell", true},
    {54, "Tambourine", false},
    {55, "Splash Cymbal", true},
    {56, "Cowbell", false},
    {57, "Crash Cymbal 2", true},
    {58, "Vibraslap", false},
    {59, "Ride Cymbal 2", false},
}};

int chokeGroupFor(int note) noexcept
{
    const bool hiHat = note == 42 || note == 44 || note == 46;
    return hiHat ? kHiHatChokeGroup : 0;
}

KitSlotDescription makeSlot(const GmDrum& drum)
{
    KitSlotDescription slot;
    slot.gmNote = drum.note;
    slot.midiNote = drum.note;
    slot.name = drum.name;
    slot.chokeGroup = chokeGroupFor(drum.note);
    slot.coreSlot = drum.core;
    return slot;
}

} // namespace

bool KitSlotDescription::usesGmNote() const noexcept
{
    return midiNote == gmNote;
}

bool isCoreGmNote(int gmNote) noexcept
{
    for (const auto& drum : kGmDrums)
        if (drum.note == gmNote)
            return drum.core;
    return true;
}

KitDescription makeGmDefaultKit()
{
    KitDescription kit;
    kit.reserve(kGmDrums.size());
    for (const auto& drum : kGmDrums)
        kit.push_back(makeSlot(drum));
    return kit;
}

} // namespace drumprog::engine
