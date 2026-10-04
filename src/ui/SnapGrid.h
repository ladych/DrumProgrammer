#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace drumprog::ui
{

/// Grid of the piano roll (F-PR-04): straight note values 1/4 to 1/32 and their triplets.
enum class GridDivision : std::uint8_t
{
    quarter,
    eighth,
    sixteenth,
    thirtySecond,
    quarterTriplet,
    eighthTriplet,
    sixteenthTriplet,
    thirtySecondTriplet
};

/// In the order of the grid dropdown.
inline constexpr std::array kGridDivisions{GridDivision::quarter,
                                           GridDivision::eighth,
                                           GridDivision::sixteenth,
                                           GridDivision::thirtySecond,
                                           GridDivision::quarterTriplet,
                                           GridDivision::eighthTriplet,
                                           GridDivision::sixteenthTriplet,
                                           GridDivision::thirtySecondTriplet};

/// Length of one grid step, e.g. 240 ticks for 1/16 and 80 ticks for 1/32 triplets at 960 PPQ.
[[nodiscard]] std::int64_t gridStepTicks(GridDivision division, int ticksPerQuarter);
/// "1/16", "1/8T", ...
[[nodiscard]] std::string gridLabel(GridDivision division);
/// Nearest grid line; halfway rounds up.
[[nodiscard]] std::int64_t snapToGrid(std::int64_t tick, std::int64_t step);
/// Grid line at or before the tick, also for negative ticks.
[[nodiscard]] std::int64_t floorToGrid(std::int64_t tick, std::int64_t step);
/// Note value of a length if it is one of the grid steps, a half or a whole note, e.g. "1/16".
[[nodiscard]] std::optional<std::string> noteValueLabel(std::int64_t ticks, int ticksPerQuarter);

} // namespace drumprog::ui
