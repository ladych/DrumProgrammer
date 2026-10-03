#pragma once

#include <cstdint>
#include <vector>

namespace drumprog::engine
{

/// Immutable copy of everything the audio thread needs from the project (Pflichtenheft chapter 4).
/// The GUI thread builds it from the model after every change; the audio thread never reads the
/// ValueTree. References are resolved to indices so the audio thread needs no lookups.

struct SlotSnapshot
{
    int midiNote = 0;
    double gain = 1.0;
    double pitch = 0.0;
    int chokeGroup = 0;

    bool operator==(const SlotSnapshot&) const = default;
};

struct NoteSnapshot
{
    int slotIndex = 0;
    std::int64_t startTick = 0;
    std::int64_t lengthTicks = 0;
    int velocity = 0;

    bool operator==(const NoteSnapshot&) const = default;
};

struct PatternSnapshot
{
    std::int64_t lengthTicks = 0;
    std::vector<NoteSnapshot> notes; ///< sorted by startTick

    bool operator==(const PatternSnapshot&) const = default;
};

struct SongEntrySnapshot
{
    int patternIndex = 0;
    std::int64_t startTick = 0;

    bool operator==(const SongEntrySnapshot&) const = default;
};

struct MixSnapshot
{
    double backingGain = 1.0;
    double drumsGain = 1.0;
    double masterGain = 1.0;

    bool operator==(const MixSnapshot&) const = default;
};

struct ProjectSnapshot
{
    double bpm = 0.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    int ticksPerQuarter = 0;
    std::vector<SlotSnapshot> slots;
    std::vector<PatternSnapshot> patterns;
    std::vector<SongEntrySnapshot> song; ///< sorted by startTick
    MixSnapshot mix;
};

} // namespace drumprog::engine
