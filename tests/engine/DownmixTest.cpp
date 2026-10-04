#include "engine/Downmix.h"

#include <gtest/gtest.h>

#include <cmath>

namespace drumprog::engine
{
namespace
{

TEST(DownmixTest, FSE01_MonoAndStereoStayUnchanged)
{
    const SampleBuffer mono{44100.0, {{0.5F, -0.5F}}};
    EXPECT_EQ(downmixToStereo(mono).channels, mono.channels);
    const SampleBuffer stereo{44100.0, {{0.5F}, {-0.5F}}};
    EXPECT_EQ(downmixToStereo(stereo).channels, stereo.channels);
}

TEST(DownmixTest, FSE01_MultichannelBecomesStereoWithEqualGain)
{
    SampleBuffer source{44100.0, {}};
    for (int channel = 0; channel < 16; ++channel)
        source.channels.push_back({0.25F, static_cast<float>(channel)});
    const auto result = downmixToStereo(source);

    ASSERT_EQ(result.numChannels(), 2);
    EXPECT_EQ(result.sampleRate, 44100.0);
    EXPECT_EQ(result.numFrames(), 2);
    EXPECT_FLOAT_EQ(result.channels[0][0], 16 * 0.25F / 4.0F);
    EXPECT_FLOAT_EQ(result.channels[0][1], 120.0F / 4.0F);
    EXPECT_EQ(result.channels[0], result.channels[1]);
}

TEST(DownmixTest, FSE01_UsesShortestChannelLength)
{
    const SampleBuffer source{48000.0, {{1.0F, 1.0F}, {1.0F}, {1.0F, 1.0F}}};
    const auto result = downmixToStereo(source);
    EXPECT_EQ(result.numFrames(), 1);
    EXPECT_NEAR(result.channels[0][0], 3.0F / std::sqrt(3.0F), 1.0e-6);
}

} // namespace
} // namespace drumprog::engine
