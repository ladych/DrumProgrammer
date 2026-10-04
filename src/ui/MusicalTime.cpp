#include "ui/MusicalTime.h"

#include "engine/TempoMath.h"

#include <iomanip>
#include <sstream>

namespace drumprog::ui
{

std::string formatPosition(std::int64_t tick, int ticksPerQuarter, model::TimeSignature signature)
{
    const std::int64_t beatTicks = engine::ticksPerBeat(ticksPerQuarter, signature.denominator);
    const std::int64_t barTicks = beatTicks * signature.numerator;
    std::ostringstream text;
    text << std::setfill('0') << std::setw(3) << tick / barTicks + 1 << '.' << tick % barTicks / beatTicks + 1
         << '.' << std::setw(3) << tick % beatTicks;
    return text.str();
}

} // namespace drumprog::ui
