#pragma once

#include "model/Project.h"

#include <cstdint>
#include <vector>

namespace drumprog::model
{

/// A song block with its pattern resolved, in ticks from the song start.
struct SongBlock
{
    int entryIndex = 0;   ///< child index in the SONG tree
    int patternIndex = 0; ///< index in the pattern list
    std::int64_t startTick = 0;
    std::int64_t lengthTicks = 0; ///< length of the pattern
    std::int64_t playedTicks = 0; ///< what is heard: up to the start of the next block
};

/// The song as it plays (F-TR-05, F-SO-04): blocks sorted by start, blocks of unknown patterns left
/// out. A block plays until its pattern ends or the next block starts, whichever comes first; of
/// blocks starting on the same bar the one added last plays. Shared by the snapshot for the audio
/// thread and the song timeline, so both show the same.
[[nodiscard]] std::vector<SongBlock> layoutSong(const Project& project);

/// End of the last block that is heard; 0 for an empty song.
[[nodiscard]] std::int64_t songLengthTicks(const std::vector<SongBlock>& blocks);

} // namespace drumprog::model
