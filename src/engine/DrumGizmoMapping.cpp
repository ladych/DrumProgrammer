#include "engine/DrumGizmoMapping.h"

#include "engine/KitDescription.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace drumprog::engine
{
namespace
{

using Match = bool (*)(const std::string&);

/// GM tom notes from the highest to the lowest tom.
constexpr std::array<int, 6> kTomNotes{50, 48, 47, 45, 43, 41};

std::string lowerCase(std::string text)
{
    std::ranges::transform(
        text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool has(const std::string& name, std::string_view part)
{
    return name.find(part) != std::string::npos;
}

bool isKick(const std::string& n)
{
    return has(n, "kick") || has(n, "kdrum") || has(n, "bassdrum") || has(n, "bass drum");
}
bool isSnare(const std::string& n)
{
    return has(n, "snare");
}
bool isRim(const std::string& n)
{
    return has(n, "stick") || has(n, "rim");
}
bool isHiHat(const std::string& n)
{
    return has(n, "hihat") || has(n, "hi-hat") || has(n, "hat") || n.starts_with("hh");
}
bool isOpenHiHat(const std::string& n)
{
    return isHiHat(n) && has(n, "open");
}
bool isPedalHiHat(const std::string& n)
{
    return isHiHat(n) && (has(n, "pedal") || has(n, "foot"));
}
bool isClosedHiHat(const std::string& n)
{
    return isHiHat(n) && (has(n, "closed") || has(n, "tight"));
}
bool isRide(const std::string& n)
{
    return has(n, "ride");
}
bool isRideBell(const std::string& n)
{
    return isRide(n) && has(n, "bell");
}
bool isTom(const std::string& n)
{
    // "Snare_bottom" is a snare microphone's instrument, not a tom.
    return has(n, "tom") && !has(n, "bottom");
}
bool isFloorTom(const std::string& n)
{
    return isTom(n) && (has(n, "floor") || has(n, "ftom"));
}

bool isSnareRim(const std::string& n)
{
    return isSnare(n) && isRim(n);
}
bool isPlainSnare(const std::string& n)
{
    return isSnare(n) && !isRim(n);
}
bool isPlainHiHat(const std::string& n)
{
    return isHiHat(n) && !isOpenHiHat(n) && !isPedalHiHat(n);
}
bool isPlainRide(const std::string& n)
{
    return isRide(n) && !isRideBell(n);
}
bool isClap(const std::string& n)
{
    return has(n, "clap");
}
bool isCrash(const std::string& n)
{
    return has(n, "crash");
}
bool isChina(const std::string& n)
{
    return has(n, "china");
}
bool isTambourine(const std::string& n)
{
    return has(n, "tamb");
}
bool isSplash(const std::string& n)
{
    return has(n, "splash");
}
bool isCowbell(const std::string& n)
{
    return has(n, "cowbell");
}
bool isVibraslap(const std::string& n)
{
    return has(n, "vibra");
}

constexpr std::array<Match, 1> kKickRules{isKick};
constexpr std::array<Match, 2> kSideStickRules{isSnareRim, isRim};
constexpr std::array<Match, 2> kSnareRules{isPlainSnare, isSnare};
constexpr std::array<Match, 1> kClapRules{isClap};
constexpr std::array<Match, 3> kClosedHiHatRules{isClosedHiHat, isPlainHiHat, isHiHat};
constexpr std::array<Match, 4> kPedalHiHatRules{isPedalHiHat, isClosedHiHat, isPlainHiHat, isHiHat};
constexpr std::array<Match, 2> kOpenHiHatRules{isOpenHiHat, isHiHat};
constexpr std::array<Match, 1> kCrashRules{isCrash};
constexpr std::array<Match, 2> kRideRules{isPlainRide, isRide};
constexpr std::array<Match, 1> kChinaRules{isChina};
constexpr std::array<Match, 2> kRideBellRules{isRideBell, isRide};
constexpr std::array<Match, 1> kTambourineRules{isTambourine};
constexpr std::array<Match, 2> kSplashRules{isSplash, isCrash};
constexpr std::array<Match, 1> kCowbellRules{isCowbell};
constexpr std::array<Match, 1> kVibraslapRules{isVibraslap};

struct NoteRules
{
    int gmNote;
    std::span<const Match> rules;
};

/// Name-based suggestions per GM note, best first; the first rule any instrument matches wins.
const std::array<NoteRules, 19> kNoteRules{{
    {35, kKickRules},      {36, kKickRules},       {37, kSideStickRules},   {38, kSnareRules},
    {39, kClapRules},      {40, kSnareRules},      {42, kClosedHiHatRules}, {44, kPedalHiHatRules},
    {46, kOpenHiHatRules}, {49, kCrashRules},      {51, kRideRules},        {52, kChinaRules},
    {53, kRideBellRules},  {54, kTambourineRules}, {55, kSplashRules},      {56, kCowbellRules},
    {57, kCrashRules},     {58, kVibraslapRules},  {59, kRideRules},
}};

std::span<const Match> rulesFor(int gmNote)
{
    for (const auto& entry : kNoteRules)
        if (entry.gmNote == gmNote)
            return entry.rules;
    return {};
}

std::vector<std::string> matching(const DrumGizmoKit& kit, Match match)
{
    std::vector<std::string> names;
    for (const auto& instrument : kit.instruments)
        if (match(lowerCase(instrument.name)))
            names.push_back(instrument.name);
    return names;
}

std::string suggestByName(const DrumGizmoKit& kit, int gmNote)
{
    for (const auto& rule : rulesFor(gmNote))
    {
        const auto names = matching(kit, rule);
        if (names.empty())
            continue;
        // The second crash or ride takes the second instrument of its kind, if there is one.
        const bool second = (gmNote == 57 || gmNote == 59) && names.size() > 1;
        return names[second ? 1 : 0];
    }
    return {};
}

/// Toms from high to low: rack toms by name, then floor toms by name.
std::vector<std::string> tomsHighToLow(const DrumGizmoKit& kit)
{
    auto rack = matching(kit, [](const std::string& n) { return isTom(n) && !isFloorTom(n); });
    auto floor = matching(kit, isFloorTom);
    std::ranges::sort(rack);
    std::ranges::sort(floor);
    rack.insert(rack.end(), floor.begin(), floor.end());
    return rack;
}

/// Which of the six GM tom notes the kit's toms take, by the number of toms.
std::vector<std::size_t> tomPositions(std::size_t toms)
{
    switch (toms)
    {
    case 1:
        return {1};
    case 2:
        return {1, 4};
    case 3:
        return {0, 2, 4};
    case 4:
        return {0, 2, 4, 5};
    case 5:
        return {0, 1, 2, 4, 5};
    default:
        return {0, 1, 2, 3, 4, 5};
    }
}

std::size_t distance(std::size_t a, std::size_t b)
{
    return a > b ? a - b : b - a;
}

/// Every GM tom note plays a tom: the kit's toms on their positions, the notes between them the nearest one.
std::map<int, std::string> mapToms(const DrumGizmoKit& kit)
{
    const auto toms = tomsHighToLow(kit);
    std::map<int, std::string> mapped;
    if (toms.empty())
        return mapped;
    const auto positions = tomPositions(toms.size());
    for (std::size_t note = 0; note < kTomNotes.size(); ++note)
    {
        std::size_t nearest = 0;
        for (std::size_t tom = 1; tom < positions.size(); ++tom)
            if (distance(positions[tom], note) < distance(positions[nearest], note))
                nearest = tom;
        mapped[kTomNotes.at(note)] = toms[nearest];
    }
    return mapped;
}

std::map<std::string, int> chokeGroupsByGroup(const DrumGizmoKit& kit)
{
    std::map<std::string, int> groups;
    int next = kHiHatChokeGroup + 1;
    for (const auto& instrument : kit.instruments)
    {
        if (instrument.group.empty() || groups.contains(instrument.group))
            continue;
        const auto group = lowerCase(instrument.group);
        groups[instrument.group] = has(group, "hat") || has(group, "hh") ? kHiHatChokeGroup : next++;
    }
    return groups;
}

} // namespace

std::map<int, DrumGizmoSlotMapping> mapToGmSlots(const DrumGizmoKit& kit)
{
    const auto toms = mapToms(kit);
    const auto groups = chokeGroupsByGroup(kit);
    std::map<int, DrumGizmoSlotMapping> mapping;
    for (const auto& slot : makeGmDefaultKit())
    {
        std::string instrument;
        if (const auto mapped = kit.midiMap.find(slot.gmNote);
            mapped != kit.midiMap.end() && kit.findInstrument(mapped->second) != nullptr)
            instrument = mapped->second;
        else if (const auto tom = toms.find(slot.gmNote); tom != toms.end())
            instrument = tom->second;
        else
            instrument = suggestByName(kit, slot.gmNote);

        const auto* found = kit.findInstrument(instrument);
        const auto group = found != nullptr ? groups.find(found->group) : groups.end();
        mapping[slot.gmNote] = {.instrument = instrument,
                                .chokeGroup = group != groups.end() ? group->second : slot.chokeGroup};
    }
    return mapping;
}

} // namespace drumprog::engine
