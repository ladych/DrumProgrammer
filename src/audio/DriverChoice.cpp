#include "audio/DriverChoice.h"

#include <algorithm>
#include <array>
#include <iterator>

namespace drumprog::audio
{
namespace
{

// The type names JUCE gives the drivers.
constexpr std::array<std::string_view, 7> kPreferredTypes{"ASIO",
                                                          "Windows Audio",
                                                          "Windows Audio (Low Latency Mode)",
                                                          "Windows Audio (Exclusive Mode)",
                                                          "DirectSound",
                                                          "ALSA",
                                                          "JACK"};

std::size_t rank(const std::string& type)
{
    return static_cast<std::size_t>(
        std::distance(kPreferredTypes.begin(), std::ranges::find(kPreferredTypes, type)));
}

constexpr std::size_t kLastRank = kPreferredTypes.size() + 1;

} // namespace

std::vector<std::string> driverTypesByPreference(const std::vector<DriverDevices>& drivers,
                                                 std::string_view failedType)
{
    std::vector<std::string> types;
    for (const auto& driver : drivers)
        if (!driver.outputs.empty())
            types.push_back(driver.type);
    std::ranges::stable_sort(types,
                             {},
                             [failedType](const std::string& type)
                             { return type == failedType ? kLastRank : rank(type); });
    return types;
}

} // namespace drumprog::audio
