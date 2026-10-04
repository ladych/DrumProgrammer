#include "engine/SampleBuffer.h"
#include "engine/Resampler.h"

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

namespace drumprog::engine
{
namespace
{

SampleBuffer sine(double sampleRate, double frequency, int frames)
{
    SampleBuffer buffer{sampleRate, {std::vector<float>(static_cast<size_t>(frames))}};
    for (int i = 0; i < frames; ++i)
        buffer.channels[0][static_cast<size_t>(i)] =
            static_cast<float>(std::sin(2.0 * std::numbers::pi * frequency * i / sampleRate));
    return buffer;
}

TEST(SampleBufferTest, FSE02_EmptyBufferHasNoFrames)
{
    const SampleBuffer buffer;
    EXPECT_EQ(buffer.numChannels(), 0);
    EXPECT_EQ(buffer.numFrames(), 0);
}

TEST(SampleBufferTest, FSE02_FramesAreLengthOfShortestChannel)
{
    const SampleBuffer buffer{44100.0, {{1.0F, 2.0F, 3.0F}, {1.0F, 2.0F}}};
    EXPECT_EQ(buffer.numChannels(), 2);
    EXPECT_EQ(buffer.numFrames(), 2);
}

TEST(ResamplerTest, FSE02_SameRateCopiesSample)
{
    const SampleBuffer source{48000.0, {{0.1F, 0.2F, 0.3F}}};
    const auto result = resample(source, 48000.0);
    EXPECT_EQ(result.sampleRate, 48000.0);
    EXPECT_EQ(result.channels, source.channels);
}

TEST(ResamplerTest, FSE02_InvalidRatesReturnCopy)
{
    const SampleBuffer source{0.0, {{0.1F, 0.2F}}};
    EXPECT_EQ(resample(source, 48000.0).channels, source.channels);
    const SampleBuffer valid{44100.0, {{0.1F, 0.2F}}};
    EXPECT_EQ(resample(valid, 0.0).channels, valid.channels);
}

TEST(ResamplerTest, FSE02_UpsamplingScalesLengthAndKeepsFirstSample)
{
    const auto source = sine(44100.0, 441.0, 441);
    const auto result = resample(source, 48000.0);
    EXPECT_EQ(result.sampleRate, 48000.0);
    EXPECT_EQ(result.numFrames(), 480);
    EXPECT_FLOAT_EQ(result.channels[0][0], source.channels[0][0]);
}

TEST(ResamplerTest, FSE02_ResampledSineMatchesSineAtTargetRate)
{
    const auto result = resample(sine(44100.0, 441.0, 4410), 48000.0);
    const auto expected = sine(48000.0, 441.0, result.numFrames());
    // Skip the last frames, where the interpolation runs past the end of the source.
    for (int i = 0; i < result.numFrames() - 4; ++i)
        EXPECT_NEAR(
            result.channels[0][static_cast<size_t>(i)], expected.channels[0][static_cast<size_t>(i)], 1.0e-3)
            << "frame " << i;
}

TEST(ResamplerTest, FSE02_DownsamplingResamplesEveryChannel)
{
    SampleBuffer source = sine(96000.0, 100.0, 960);
    source.channels.push_back(source.channels[0]);
    const auto result = resample(source, 48000.0);
    ASSERT_EQ(result.numChannels(), 2);
    EXPECT_EQ(result.numFrames(), 480);
    EXPECT_EQ(result.channels[0], result.channels[1]);
    EXPECT_NEAR(result.channels[0][120], 1.0F, 1.0e-3);
}

TEST(ResamplerTest, FSE02_EmptySampleStaysEmpty)
{
    const SampleBuffer source{44100.0, {{}}};
    const auto result = resample(source, 48000.0);
    EXPECT_EQ(result.numFrames(), 0);
}

} // namespace
} // namespace drumprog::engine
