#include "ui/SnapGrid.h"

#include "engine/TempoMath.h"

#include <cstddef>

namespace drumprog::ui
{
namespace
{

struct DivisionSpec
{
    int quarters; ///< a step is quarters / parts of a quarter note
    int parts;
    const char* label;
};

/// Indexed by GridDivision.
constexpr std::array<DivisionSpec, kGridDivisions.size()> kSpecs{{{1, 1, "1/4"},
                                                                  {1, 2, "1/8"},
                                                                  {1, 4, "1/16"},
                                                                  {1, 8, "1/32"},
                                                                  {2, 3, "1/4T"},
                                                                  {1, 3, "1/8T"},
                                                                  {1, 6, "1/16T"},
                                                                  {1, 12, "1/32T"}}};

const DivisionSpec& specOf(GridDivision division)
{
    return kSpecs.at(static_cast<std::size_t>(division));
}

} // namespace

std::int64_t gridStepTicks(GridDivision division, int ticksPerQuarter)
{
    const auto& spec = specOf(division);
    return std::int64_t{ticksPerQuarter} * spec.quarters / spec.parts;
}

std::string gridLabel(GridDivision division)
{
    return specOf(division).label;
}

std::int64_t snapToGrid(std::int64_t tick, std::int64_t step)
{
    return floorToGrid(tick + step / 2, step);
}

std::int64_t floorToGrid(std::int64_t tick, std::int64_t step)
{
    return engine::floorDiv(tick, step) * step;
}

std::optional<std::string> noteValueLabel(std::int64_t ticks, int ticksPerQuarter)
{
    if (ticks == std::int64_t{ticksPerQuarter} * 4)
        return "1/1";
    if (ticks == std::int64_t{ticksPerQuarter} * 2)
        return "1/2";
    for (const auto division : kGridDivisions)
        if (ticks == gridStepTicks(division, ticksPerQuarter))
            return gridLabel(division);
    return std::nullopt;
}

} // namespace drumprog::ui
