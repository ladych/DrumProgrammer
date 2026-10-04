#pragma once

namespace drumprog::engine
{

/// Monotonic time in seconds, read by the input threads for each hit and by the audio thread for
/// each block, so a recorded hit can be placed where it was played (F-IN-08). A plain function
/// pointer: safe to call on every thread, and tests inject a fake one.
using Clock = double (*)() noexcept;

/// std::chrono::steady_clock in seconds.
[[nodiscard]] double steadyClockSeconds() noexcept;

} // namespace drumprog::engine
