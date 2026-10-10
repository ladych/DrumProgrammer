#include "audio/DriverChoice.h"

#include <gtest/gtest.h>

namespace drumprog::audio
{
namespace
{

using Types = std::vector<std::string>;

// The driver types in the order JUCE lists them on Windows.
std::vector<DriverDevices> windowsWithAsio()
{
    return {{.type = "Windows Audio", .outputs = {"Lautsprecher", "Scarlett 2i2"}},
            {.type = "Windows Audio (Exclusive Mode)", .outputs = {"Lautsprecher", "Scarlett 2i2"}},
            {.type = "Windows Audio (Low Latency Mode)", .outputs = {"Lautsprecher", "Scarlett 2i2"}},
            {.type = "DirectSound", .outputs = {"Primary Sound Driver"}},
            {.type = "ASIO", .outputs = {"Focusrite USB ASIO"}}};
}

TEST(DriverChoiceTest, FAO03_AsioComesFirstOnWindows)
{
    EXPECT_EQ(driverTypesByPreference(windowsWithAsio()),
              (Types{"ASIO",
                     "Windows Audio",
                     "Windows Audio (Low Latency Mode)",
                     "Windows Audio (Exclusive Mode)",
                     "DirectSound"}));
}

TEST(DriverChoiceTest, FAO03_WithoutAsioDriverWasapiComesFirst)
{
    auto drivers = windowsWithAsio();
    drivers.back().outputs.clear();
    EXPECT_EQ(driverTypesByPreference(drivers).front(), "Windows Audio");
}

TEST(DriverChoiceTest, FAO03_TheFailedTypeComesLast)
{
    const auto types = driverTypesByPreference(windowsWithAsio(), "ASIO");
    EXPECT_EQ(types.front(), "Windows Audio");
    EXPECT_EQ(types.back(), "ASIO");
}

TEST(DriverChoiceTest, Q09_UnpluggedAlsaDeviceFallsBackToAnotherAlsaDeviceLast)
{
    const std::vector<DriverDevices> linux{{.type = "ALSA", .outputs = {"default", "Scarlett 2i2"}},
                                           {.type = "JACK"}};
    EXPECT_EQ(driverTypesByPreference(linux, "ALSA"), (Types{"ALSA"}));
}

TEST(DriverChoiceTest, FAO02_LinuxKeepsAlsaBeforeJack)
{
    const std::vector<DriverDevices> linux{{.type = "ALSA", .outputs = {"default"}},
                                           {.type = "JACK", .outputs = {"system"}}};
    EXPECT_EQ(driverTypesByPreference(linux), (Types{"ALSA", "JACK"}));
}

TEST(DriverChoiceTest, FAO02_JackWithoutRunningServerIsSkipped)
{
    const std::vector<DriverDevices> linux{{.type = "ALSA", .outputs = {"default"}}, {.type = "JACK"}};
    EXPECT_EQ(driverTypesByPreference(linux), (Types{"ALSA"}));
}

TEST(DriverChoiceTest, FAO03_UnknownTypesFollowInTheirOwnOrder)
{
    const std::vector<DriverDevices> drivers{{.type = "Bela", .outputs = {"b"}},
                                             {.type = "CoreAudio", .outputs = {"c"}},
                                             {.type = "ALSA", .outputs = {"a"}}};
    EXPECT_EQ(driverTypesByPreference(drivers), (Types{"ALSA", "Bela", "CoreAudio"}));
}

TEST(DriverChoiceTest, Q09_NoDevicesGiveNoType)
{
    EXPECT_TRUE(driverTypesByPreference({{.type = "ALSA"}}).empty());
}

} // namespace
} // namespace drumprog::audio
