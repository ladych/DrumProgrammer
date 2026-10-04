#include "model/ProjectFactory.h"

#include "model/ModelIds.h"
#include "model/Project.h"

#include <array>
#include <string_view>

namespace drumprog::model
{
namespace
{

constexpr const char* kDefaultProjectName = "Neues Projekt";
constexpr const char* kFirstPatternName = "Pattern 1";

/// General MIDI percussion names for notes 35-59.
constexpr std::array<std::string_view, ProjectFactory::kLastGmNote - ProjectFactory::kFirstGmNote + 1>
    kGmDrumNames{
        "Acoustic Bass Drum", "Bass Drum 1",    "Side Stick",     "Acoustic Snare",
        "Hand Clap",          "Electric Snare", "Low Floor Tom",  "Closed Hi-Hat",
        "High Floor Tom",     "Pedal Hi-Hat",   "Low Tom",        "Open Hi-Hat",
        "Low-Mid Tom",        "Hi-Mid Tom",     "Crash Cymbal 1", "High Tom",
        "Ride Cymbal 1",      "Chinese Cymbal", "Ride Bell",      "Tambourine",
        "Splash Cymbal",      "Cowbell",        "Crash Cymbal 2", "Vibraslap",
        "Ride Cymbal 2",
    };

constexpr bool isHiHat(int gmNote)
{
    constexpr int kClosedHiHat = 42;
    constexpr int kPedalHiHat = 44;
    constexpr int kOpenHiHat = 46;
    return gmNote == kClosedHiHat || gmNote == kPedalHiHat || gmNote == kOpenHiHat;
}

} // namespace

ProjectFactory::ProjectFactory(IIdGenerator& idGenerator) : idGenerator_(idGenerator) {}

juce::ValueTree ProjectFactory::createDefault() const
{
    juce::ValueTree tree{ids::project};
    tree.setProperty(ids::name, kDefaultProjectName, nullptr);
    tree.setProperty(ids::bpm, kDefaultBpm, nullptr);
    tree.setProperty(ids::timeSigNumerator, 4, nullptr);
    tree.setProperty(ids::timeSigDenominator, 4, nullptr);
    tree.setProperty(ids::ppq, kTicksPerQuarter, nullptr);
    tree.appendChild(createDefaultKit(), nullptr);
    tree.appendChild(juce::ValueTree{ids::patterns}, nullptr);
    tree.appendChild(juce::ValueTree{ids::song}, nullptr);
    tree.appendChild(juce::ValueTree{ids::backingTrack}, nullptr);
    tree.appendChild(juce::ValueTree{ids::mix}, nullptr);

    Project{tree, nullptr}.addPattern(idGenerator_.next(), kFirstPatternName, kDefaultPatternBars);
    return tree;
}

juce::ValueTree ProjectFactory::createDefaultKit()
{
    juce::ValueTree kit{ids::kit};
    for (int gmNote = kFirstGmNote; gmNote <= kLastGmNote; ++gmNote)
    {
        const auto name = kGmDrumNames.at(static_cast<std::size_t>(gmNote - kFirstGmNote));
        juce::ValueTree slot{ids::slot};
        slot.setProperty(ids::gmNote, gmNote, nullptr);
        slot.setProperty(ids::midiNote, gmNote, nullptr);
        slot.setProperty(ids::name, juce::String{name.data(), name.size()}, nullptr);
        slot.setProperty(ids::chokeGroup, isHiHat(gmNote) ? kHiHatChokeGroup : 0, nullptr);
        kit.appendChild(slot, nullptr);
    }
    return kit;
}

} // namespace drumprog::model
