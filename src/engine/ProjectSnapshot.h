#pragma once

#include <cstdint>
#include <vector>

namespace drumprog::engine
{

/// Immutable copy of the project's timing data for the audio thread (Pflichtenheft chapter 4).
/// The GUI thread builds it from the model after every change; the audio thread never reads the
/// ValueTree. References are resolved to indices so the audio thread needs no lookups.
///
/// The kit is not part of it: slots reach the audio thread as EngineKit through the SampleEngine,
/// because their samples have to stay alive until the last voice playing them has ended.

struct NoteSnapshot
{
    int slotIndex = 0; ///< index into the kit, same order as the slots of the EngineKit
    std::int64_t startTick = 0;
    std::int64_t lengthTicks = 0;
    int velocity = 0;
};

struct PatternSnapshot
{
    std::int64_t lengthTicks = 0;
    std::vector<NoteSnapshot> notes; ///< sorted by startTick
};

struct SongEntrySnapshot
{
    int patternIndex = 0;
    std::int64_t startTick = 0;
    std::int64_t lengthTicks = 0; ///< played length: up to the next block at most
};

struct BackingTrackSnapshot
{
    std::int64_t offsetSamples = 0; ///< file frame at the song start (F-BT-05)
    double gain = 1.0;
};

/// Linear gains of the faders "Mix (Referenz)" (F-BT-06).
struct MixSnapshot
{
    double backingGain = 1.0;
    double drumsGain = 1.0;
    double masterGain = 1.0;
};

struct ProjectSnapshot
{
    double bpm = 0.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    int ticksPerQuarter = 0;
    std::vector<PatternSnapshot> patterns;
    std::vector<SongEntrySnapshot> song; ///< sorted by startTick
    std::int64_t songLengthTicks = 0;    ///< end of the last block
    BackingTrackSnapshot backingTrack;
    MixSnapshot mix;
};

} // namespace drumprog::engine
