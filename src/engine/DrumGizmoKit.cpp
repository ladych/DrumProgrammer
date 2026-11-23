#include "engine/DrumGizmoKit.h"

#include <algorithm>

namespace drumprog::engine
{

const DrumGizmoInstrument* DrumGizmoKit::findInstrument(const std::string& instrumentName) const
{
    const auto found = std::ranges::find(instruments, instrumentName, &DrumGizmoInstrument::name);
    return found != instruments.end() ? &*found : nullptr;
}

} // namespace drumprog::engine
