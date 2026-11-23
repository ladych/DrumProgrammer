#pragma once

#include "engine/DrumGizmoKit.h"

#include <map>
#include <string>

namespace drumprog::engine
{

/// What a GM slot plays from a DrumGizmo kit.
struct DrumGizmoSlotMapping
{
    std::string instrument; ///< empty: nothing fits
    int chokeGroup = 0;
};

/// Suggests an instrument for every slot of the GM default kit (F-DG-07): the kit's MIDI map first, then the
/// instrument names (kick, snare, hihat, tom, crash, ride ...). Toms are spread over the six GM tom notes
/// from high to low, missing hi-hat, crash or ride variants fall back to the plain instrument. Instruments of
/// one DrumGizmo group choke each other (F-DG-08): a hi-hat group gets the hi-hat choke group, other groups
/// the following numbers; slots without a group keep the GM default.
[[nodiscard]] std::map<int, DrumGizmoSlotMapping> mapToGmSlots(const DrumGizmoKit& kit);

} // namespace drumprog::engine
