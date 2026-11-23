#pragma once

#include <cstdint>
#include <string_view>

namespace drumprog::engine
{

/// Where a microphone of a multi-microphone kit goes in the stereo mix (F-DG-03).
enum class MicPosition : std::uint8_t
{
    centre,
    left,
    right,
    muted,
};

/// Overhead and ambience/room microphones go left or right by the side in their name ("OHL", "AmbR",
/// "Room_Left"), trigger channels are muted, every other microphone (kick, snare, toms ...) is centred.
[[nodiscard]] MicPosition micPosition(std::string_view channelName);

struct MicGains
{
    float left = 0.0F;
    float right = 0.0F;
};

/// Gains of one microphone when activeMics microphones are mixed: each one gets 1/sqrt(activeMics) like the
/// plain downmix (engine/Downmix.h), panned ones the same power on one side only.
[[nodiscard]] MicGains micGains(MicPosition position, int activeMics) noexcept;

} // namespace drumprog::engine
