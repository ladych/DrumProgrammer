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

TEST(AudioStatusTest, Q03_ShowsDropoutsWhenTheDriverCountsThem)
{
    auto info = jack128();
    info.xruns = 3;
    EXPECT_EQ(
        audioStatusText(info),
        "JACK | system | 48000 Hz | Buffer 128 (2,7 ms) | Latenz 5,3 ms, Anschlag bis Ton max. 8,0 ms | "
        "XRuns: 3 | MIDI: Alesis Nitro");
    info.xruns = 0;
    EXPECT_NE(audioStatusText(info).find("| XRuns: 0 |"), std::string::npos);
}

TEST(AudioStatusTest, Q09_WarnsWhileTheDeviceGivesNoAudio)
{
    auto info = jack128();
    info.stalled = true;
    EXPECT_EQ(audioStatusText(info).rfind("Audio-Gerät antwortet nicht, neuer Versuch läuft | JACK | ", 0),
              0U);
    AudioDeviceInfo closed;
    closed.stalled = true;
    EXPECT_EQ(
        audioStatusText(closed),
        "Audio-Gerät antwortet nicht, neuer Versuch läuft | Kein Audio-Gerät geöffnet | MIDI: kein Eingang");
}

TEST(AudioStatusTest, FAO03_NamesTheDeviceAFallbackReplaces)
{
    auto info = jack128();
    info.driver = "Windows Audio";
    info.device = "Lautsprecher";
    info.replacedDevice = "ASIO: Focusrite USB ASIO";
    EXPECT_EQ(audioStatusText(info).rfind(
                  "Ersatz für ASIO: Focusrite USB ASIO | Windows Audio | Lautsprecher | ", 0),
              0U);
}

} // namespace
} // namespace drumprog::ui
