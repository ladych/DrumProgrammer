#include "model/SongLayout.h"

#include "model/ModelIds.h"

#include <algorithm>

namespace drumprog::model
{

std::vector<SongBlock> layoutSong(const Project& project)
{
    std::vector<SongBlock> blocks;
    const auto song = project.song();
    const auto patterns = project.tree().getChildWithName(ids::patterns);
    const std::int64_t ticksPerBar = project.ticksPerBar();
    for (int index = 0; index < song.numEntries(); ++index)
    {
        const auto entry = song.entry(index);
        if (const auto pattern = project.findPattern(entry.patternId()))
        {
            const std::int64_t length = pattern->lengthBars() * ticksPerBar;
            blocks.push_back({.entryIndex = index,
                              .patternIndex = patterns.indexOf(pattern->tree()),
                              .startTick = entry.startBar() * ticksPerBar,
                              .lengthTicks = length,
                              .playedTicks = length});
        }
    }
    std::ranges::stable_sort(blocks, {}, &SongBlock::startTick);
    for (std::size_t index = 1; index < blocks.size(); ++index)
    {
        auto& previous = blocks[index - 1];
        previous.playedTicks = std::min(previous.playedTicks, blocks[index].startTick - previous.startTick);
    }
    return blocks;
}

std::int64_t songLengthTicks(const std::vector<SongBlock>& blocks)
{
    std::int64_t length = 0;
    for (const auto& block : blocks)
        length = std::max(length, block.startTick + block.playedTicks);
    return length;
}

} // namespace drumprog::model
