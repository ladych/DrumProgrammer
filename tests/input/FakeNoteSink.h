#pragma once

#include "input/INoteSink.h"

#include <utility>
#include <vector>

namespace drumprog::input
{

/// Records every note-on as (note, velocity).
class FakeNoteSink final : public INoteSink
{
public:
    bool noteOn(int midiNote, int velocity) noexcept override
    {
        hits.emplace_back(midiNote, velocity);
        return true;
    }

    std::vector<std::pair<int, int>> hits;
};

} // namespace drumprog::input
