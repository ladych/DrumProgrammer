#include "engine/Voice.h"

#include "engine/RenderHelpers.h"

#include <gtest/gtest.h>

#include <array>

namespace drumprog::engine
{
namespace
{

const SampleBuffer kMono{48000.0, {{1.0F, 0.5F, 0.25F, 0.125F}}};
const SampleBuffer kStereo{48000.0, {{1.0F, 1.0F}, {-1.0F, -1.0F}}};
const SampleBuffer kConstant{48000.0, {std::vector<float>(100, 1.0F)}};

TEST(VoiceTest, FSE03_IsInactiveUntilStarted)
{
    Voice voice;
    EXPECT_FALSE(voice.isActive());
    StereoOutput out(4);
    voice.render(out.channels.data(), 2, 4);
    EXPECT_EQ(out.left, std::vector<float>(4, 0.0F));
}

TEST(VoiceTest, FSE03_PlaysMonoSampleOnAllChannelsAndStopsAtEnd)
{
    Voice voice;
    voice.start({.sample = &kMono, .gain = 0.5F}, 7);
    EXPECT_TRUE(voice.isActive());
    EXPECT_EQ(voice.startOrder(), 7U);
    StereoOutput out(6);
    voice.render(out.channels.data(), 2, 6);
    const std::vector<float> expected{0.5F, 0.25F, 0.125F, 0.0625F, 0.0F, 0.0F};
    EXPECT_EQ(out.left, expected);
    EXPECT_EQ(out.right, expected);
    EXPECT_FALSE(voice.isActive());
}

TEST(VoiceTest, FSE03_AddsToExistingOutput)
{
    Voice voice;
    voice.start({.sample = &kMono}, 0);
    StereoOutput out(1);
    out.left[0] = 1.0F;
    voice.render(out.channels.data(), 2, 1);
    EXPECT_FLOAT_EQ(out.left[0], 2.0F);
}

TEST(VoiceTest, FSE03_PlaysStereoChannelsSeparatelyAndRepeatsLastChannel)
{
    Voice voice;
    voice.start({.sample = &kStereo}, 0);
    std::array<float, 2> a{};
    std::array<float, 2> b{};
    std::array<float, 2> c{};
    std::array<float*, 4> outputs{a.data(), b.data(), nullptr, c.data()};
    voice.render(outputs.data(), 4, 2);
    EXPECT_FLOAT_EQ(a[0], 1.0F);
    EXPECT_FLOAT_EQ(b[0], -1.0F);
    EXPECT_FLOAT_EQ(c[1], -1.0F);
}

TEST(VoiceTest, FTR06_StartsAtSampleOffsetInFirstBlockOnly)
{
    Voice voice;
    voice.start({.sample = &kMono, .startOffset = 2}, 0);
    StereoOutput first(3);
    voice.render(first.channels.data(), 2, 3);
    EXPECT_EQ(first.left, (std::vector<float>{0.0F, 0.0F, 1.0F}));
    StereoOutput second(2);
    voice.render(second.channels.data(), 2, 2);
    EXPECT_EQ(second.left, (std::vector<float>{0.5F, 0.25F}));
}

TEST(VoiceTest, FTR06_OffsetBeyondBlockCarriesOver)
{
    Voice voice;
    voice.start({.sample = &kMono, .startOffset = 5}, 0);
    StereoOutput first(4);
    voice.render(first.channels.data(), 2, 4);
    EXPECT_EQ(first.left, std::vector<float>(4, 0.0F));
    StereoOutput second(2);
    voice.render(second.channels.data(), 2, 2);
    EXPECT_EQ(second.left, (std::vector<float>{0.0F, 1.0F}));
}

TEST(VoiceTest, FSE07_DoubleSpeedSkipsEverySecondFrame)
{
    Voice voice;
    voice.start({.sample = &kMono, .playbackRate = 2.0}, 0);
    StereoOutput out(3);
    voice.render(out.channels.data(), 2, 3);
    EXPECT_EQ(out.left, (std::vector<float>{1.0F, 0.25F, 0.0F}));
    EXPECT_FALSE(voice.isActive());
}

TEST(VoiceTest, FSE07_HalfSpeedInterpolatesBetweenFrames)
{
    Voice voice;
    voice.start({.sample = &kMono, .playbackRate = 0.5}, 0);
    StereoOutput out(4);
    voice.render(out.channels.data(), 2, 4);
    EXPECT_EQ(out.left, (std::vector<float>{1.0F, 0.75F, 0.5F, 0.375F}));
}

TEST(VoiceTest, FSE07_InterpolationFadesToZeroAfterLastFrame)
{
    Voice voice;
    voice.start({.sample = &kMono, .playbackRate = 0.5}, 0);
    StereoOutput out(10);
    voice.render(out.channels.data(), 2, 10);
    EXPECT_FLOAT_EQ(out.left[6], 0.125F);
    EXPECT_FLOAT_EQ(out.left[7], 0.0625F);
    EXPECT_FALSE(voice.isActive());
}

TEST(VoiceTest, FSE08_ReleaseFadesOutLinearlyAndStops)
{
    Voice voice;
    voice.start({.sample = &kConstant, .chokeGroup = 1}, 0);
    EXPECT_EQ(voice.chokeGroup(), 1);
    voice.release(4);
    EXPECT_TRUE(voice.isReleasing());
    StereoOutput out(6);
    voice.render(out.channels.data(), 2, 6);
    EXPECT_EQ(out.left, (std::vector<float>{1.0F, 0.75F, 0.5F, 0.25F, 0.0F, 0.0F}));
    EXPECT_FALSE(voice.isActive());
}

TEST(VoiceTest, FSE08_SecondReleaseKeepsShorterFade)
{
    Voice voice;
    voice.start({.sample = &kConstant}, 0);
    voice.release(2);
    voice.release(10);
    StereoOutput out(3);
    voice.render(out.channels.data(), 2, 3);
    EXPECT_EQ(out.left, (std::vector<float>{1.0F, 0.5F, 0.0F}));
}

TEST(VoiceTest, FSE08_ShorterSecondReleaseReplacesLongerFade)
{
    Voice voice;
    voice.start({.sample = &kConstant}, 0);
    voice.release(10);
    voice.release(1);
    StereoOutput out(2);
    voice.render(out.channels.data(), 2, 2);
    EXPECT_EQ(out.left, (std::vector<float>{1.0F, 0.0F}));
}

TEST(VoiceTest, FSE03_ReleaseOfInactiveVoiceDoesNothing)
{
    Voice voice;
    voice.release(4);
    EXPECT_FALSE(voice.isActive());
    EXPECT_FALSE(voice.isReleasing());
}

TEST(VoiceTest, FSE03_ZeroLengthReleaseStopsImmediately)
{
    Voice voice;
    voice.start({.sample = &kConstant}, 0);
    voice.release(0);
    EXPECT_FALSE(voice.isActive());
}

TEST(VoiceTest, FSE03_StopSilencesVoiceAndRestartClearsRelease)
{
    Voice voice;
    int owner = 0;
    voice.start({.sample = &kConstant, .owner = &owner}, 0);
    EXPECT_EQ(voice.owner(), &owner);
    voice.release(4);
    voice.stop();
    EXPECT_FALSE(voice.isActive());
    voice.start({.sample = &kConstant}, 1);
    EXPECT_FALSE(voice.isReleasing());
}

TEST(VoiceTest, FSE03_EmptyOrMissingSampleDoesNotStart)
{
    Voice voice;
    voice.start({.sample = nullptr}, 0);
    EXPECT_FALSE(voice.isActive());
    const SampleBuffer empty{48000.0, {{}}};
    voice.start({.sample = &empty}, 0);
    EXPECT_FALSE(voice.isActive());
}

} // namespace
} // namespace drumprog::engine
