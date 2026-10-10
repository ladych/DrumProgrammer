#pragma once

#include "engine/ProjectSnapshot.h"
#include "io/IProjectRepository.h"
#include "model/Project.h"

#include <tuple>

// Equality for value types that production code never compares. Defined here instead of as
// defaulted members, so the coverage gate does not measure comparison branches only tests use.

namespace drumprog::engine
{

inline bool operator==(const NoteSnapshot& a, const NoteSnapshot& b)
{
    return std::tie(a.slotIndex, a.startTick, a.lengthTicks, a.velocity) ==
           std::tie(b.slotIndex, b.startTick, b.lengthTicks, b.velocity);
}

inline bool operator==(const SongEntrySnapshot& a, const SongEntrySnapshot& b)
{
    return std::tie(a.patternIndex, a.startTick, a.lengthTicks) ==
           std::tie(b.patternIndex, b.startTick, b.lengthTicks);
}

inline bool operator==(const MixSnapshot& a, const MixSnapshot& b)
{
    return std::tie(a.backingGain, a.drumsGain, a.masterGain) ==
           std::tie(b.backingGain, b.drumsGain, b.masterGain);
}

} // namespace drumprog::engine

namespace drumprog::model
{

inline bool operator==(const TimeSignature& a, const TimeSignature& b)
{
    return std::tie(a.numerator, a.denominator) == std::tie(b.numerator, b.denominator);
}

inline bool operator==(const NoteData& a, const NoteData& b)
{
    return std::tie(a.slotNote, a.startTick, a.lengthTicks, a.velocity, a.origin) ==
           std::tie(b.slotNote, b.startTick, b.lengthTicks, b.velocity, b.origin);
}

} // namespace drumprog::model

namespace drumprog::io
{

inline bool operator==(const MissingSample& a, const MissingSample& b)
{
    return std::tie(a.slotName, a.filePath) == std::tie(b.slotName, b.filePath);
}

} // namespace drumprog::io
