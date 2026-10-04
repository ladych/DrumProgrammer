#include "ui/AudioStatus.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

AudioDeviceInfo jack128()
{
    return {.open = true,
            .driver = "JACK",
            .device = "system",
            .sampleRate = 48000.0,
            .bufferSize = 128,
            .outputLatencySamples = 256,
            .midiInputs = {"Alesis Nitro"}};
}

TEST(AudioStatusTest, Q02_OutputLatencyIsTheDriversValue)
{
    EXPECT_NEAR(outputLatencyMs(jack128()), 5.333, 1.0e-3);
}

TEST(AudioStatusTest, Q02_WithoutDriverValueTheOutputLatencyIsOneBuffer)
{
    auto info = jack128();
    info.outputLatencySamples = 0;
    EXPECT_NEAR(outputLatencyMs(info), 2.667, 1.0e-3);
}

TEST(AudioStatusTest, Q01_HitToSoundAddsOneBufferOfWaiting)
{
    EXPECT_NEAR(hitToSoundLatencyMs(jack128()), 8.0, 1.0e-9);
}

TEST(AudioStatusTest, Q02_StatusLineShowsDeviceBufferAndLatency)
{
    EXPECT_EQ(
        audioStatusText(jack128()),
        "JACK | system | 48000 Hz | Buffer 128 (2,7 ms) | Latenz 5,3 ms, Anschlag bis Ton max. 8,0 ms | "
        "MIDI: Alesis Nitro");
}

TEST(AudioStatusTest, FAO05_ListsAllMidiInputs)
{
    auto info = jack128();
    info.midiInputs = {"Nitro", "Keystation"};
    EXPECT_NE(audioStatusText(info).find("MIDI: Nitro, Keystation"), std::string::npos);
    info.midiInputs.clear();
    EXPECT_NE(audioStatusText(info).find("MIDI: kein Eingang"), std::string::npos);
}

TEST(AudioStatusTest, Q02_ClosedDevice)
{
    AudioDeviceInfo info;
    EXPECT_EQ(audioStatusText(info), "Kein Audio-Gerät geöffnet | MIDI: kein Eingang");
    info.open = true;
    EXPECT_EQ(audioStatusText(info), "Kein Audio-Gerät geöffnet | MIDI: kein Eingang");
}

} // namespace
} // namespace drumprog::ui
