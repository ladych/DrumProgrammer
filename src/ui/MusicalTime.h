#pragma once

#include "model/Project.h"

#include <cstdint>
#include <string>

namespace drumprog::ui
{

/// Bar.Beat.Tick of a tick counted from the pattern start, e.g. "002.2.000" for tick 4800 in 4/4 at
/// 960 PPQ (F-TR-04, F-PR-06). Bars and beats count from 1.
[[nodiscard]] std::string
formatPosition(std::int64_t tick, int ticksPerQuarter, model::TimeSignature signature);

} // namespace drumprog::ui
