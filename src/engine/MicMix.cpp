#include "engine/MicMix.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <numbers>
#include <string>

namespace drumprog::engine
{
namespace
{

std::string lowerCase(std::string_view text)
{
    std::string lower(text);
    std::ranges::transform(
        lower, lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower;
}

bool contains(const std::string& text, std::string_view part)
{
    return text.find(part) != std::string::npos;
}

bool isStereoPairMic(const std::string& name)
{
    constexpr std::array<std::string_view, 4> kPairs{"oh", "overhead", "amb", "room"};
    return std::ranges::any_of(kPairs, [&name](std::string_view pair) { return contains(name, pair); });
}

MicPosition sideOf(const std::string& name)
{
    if (name.ends_with("left") || name.ends_with('l'))
        return MicPosition::left;
    if (name.ends_with("right") || name.ends_with('r'))
        return MicPosition::right;
    return MicPosition::centre;
}

} // namespace

MicPosition micPosition(std::string_view channelName)
{
    auto name = lowerCase(channelName);
    if (contains(name, "trig"))
        return MicPosition::muted;
    if (!isStereoPairMic(name))
        return MicPosition::centre;
    // "OH_L", "Amb-R", "Room 1 L": separators do not matter for the side.
    std::erase_if(name, [](unsigned char c) { return std::isalnum(c) == 0; });
    return sideOf(name);
}

MicGains micGains(MicPosition position, int activeMics) noexcept
{
    const float gain = 1.0F / std::sqrt(static_cast<float>(std::max(activeMics, 1)));
    const float panned = gain * std::numbers::sqrt2_v<float>;
    switch (position)
    {
    case MicPosition::left:
        return {.left = panned, .right = 0.0F};
    case MicPosition::right:
        return {.left = 0.0F, .right = panned};
    case MicPosition::muted:
        return {};
    default:
        return {.left = gain, .right = gain};
    }
}

} // namespace drumprog::engine
