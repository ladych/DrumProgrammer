#include "engine/Clock.h"

#include <chrono>

namespace drumprog::engine
{

double steadyClockSeconds() noexcept
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

} // namespace drumprog::engine
