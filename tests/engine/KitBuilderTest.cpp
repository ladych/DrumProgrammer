#include "engine/KitBuilder.h"

#include "engine/MockSampleLoader.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>

namespace drumprog::engine
{
namespace
{

using ::testing::Return;

const SampleBuffer kKick{48000.0, {{1.0F, 0.5F, 0.25F, 0.0F}}};

KitDescription kitWithKick()
{
    return {KitSlotDescription{.gmNote = 36, .midiNote = 36, .name = "Kick", .sampleFile = "kick.wav"},
            KitSlotDescription{.gmNote = 38, .midiNote = 38, .name = "Snare"}};
}

class KitBuilderTest : public ::testing::Test
{
protected:
    MockSampleLoader loader;
    KitBuilder builder{loader};
};

TEST_F(KitBuilderTest, FSE01_LoadsAssignedSamplesThroughLoader)
{
    EXPECT_CALL(loader, load(std::filesystem::path{"kick.wav"})).WillOnce(Return(kKick));
    const auto result = builder.build(kitWithKick(), 48000.0);
    ASSERT_NE(result.kit, nullptr);
    EXPECT_TRUE(result.missingSamples.empty());
    const auto* kick = result.kit->slotForNote(36);
    ASSERT_NE(kick, nullptr);
    ASSERT_NE(kick->sample, nullptr);
    EXPECT_EQ(kick->sample->channels, kKick.channels);
    const auto* snare = result.kit->slotForNote(38);
    ASSERT_NE(snare, nullptr);
    EXPECT_EQ(snare->sample, nullptr);
    EXPECT_EQ(snare->slotIndex, 1);
}

TEST_F(KitBuilderTest, FPJ03_ReportsMissingSamplesAndBuildsKitWithoutThem)
{
    EXPECT_CALL(loader, load(std::filesystem::path{"kick.wav"})).WillOnce(Return(std::nullopt));
    const auto result = builder.build(kitWithKick(), 48000.0);
    ASSERT_EQ(result.missingSamples.size(), 1U);
    EXPECT_EQ(result.missingSamples[0], std::filesystem::path{"kick.wav"});
    EXPECT_EQ(result.kit->slotForNote(36)->sample, nullptr);
}

TEST_F(KitBuilderTest, FSE02_ResamplesToDeviceRate)
{
    EXPECT_CALL(loader, load(std::filesystem::path{"kick.wav"})).WillOnce(Return(kKick));
    const auto result = builder.build(kitWithKick(), 96000.0);
    const auto& sample = *result.kit->slotForNote(36)->sample;
    EXPECT_EQ(sample.sampleRate, 96000.0);
    EXPECT_EQ(sample.numFrames(), 8);
}

TEST_F(KitBuilderTest, FSE02_ReusesLoadedSamplesAcrossBuildsAndRateChanges)
{
    EXPECT_CALL(loader, load(std::filesystem::path{"kick.wav"})).Times(1).WillOnce(Return(kKick));
    const auto first = builder.build(kitWithKick(), 48000.0);
    const auto second = builder.build(kitWithKick(), 48000.0);
    EXPECT_EQ(first.kit->slotForNote(36)->sample, second.kit->slotForNote(36)->sample);
    const auto third = builder.build(kitWithKick(), 44100.0);
    EXPECT_EQ(third.kit->slotForNote(36)->sample->sampleRate, 44100.0);
}

TEST_F(KitBuilderTest, FPJ03_RetriesMissingSampleOnNextBuild)
{
    EXPECT_CALL(loader, load(std::filesystem::path{"kick.wav"}))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(kKick));
    EXPECT_EQ(builder.build(kitWithKick(), 48000.0).kit->slotForNote(36)->sample, nullptr);
    EXPECT_NE(builder.build(kitWithKick(), 48000.0).kit->slotForNote(36)->sample, nullptr);
}

TEST_F(KitBuilderTest, FSE06_FSE07_FSE08_CopiesGainPitchAndChokeGroup)
{
    KitDescription kit{
        KitSlotDescription{.midiNote = 46, .gain = 0.5F, .pitchSemitones = 12, .chokeGroup = 1}};
    const auto result = builder.build(kit, 48000.0);
    const auto* slot = result.kit->slotForNote(46);
    EXPECT_FLOAT_EQ(slot->gain, 0.5F);
    EXPECT_DOUBLE_EQ(slot->playbackRate, 2.0);
    EXPECT_EQ(slot->chokeGroup, 1);
}

TEST_F(KitBuilderTest, FSE01_MixesMultichannelSamplesDownToStereo)
{
    const SampleBuffer multi{48000.0, {{1.0F}, {1.0F}, {1.0F}, {1.0F}}};
    EXPECT_CALL(loader, load(std::filesystem::path{"kick.wav"})).WillOnce(Return(multi));
    const auto result = builder.build(kitWithKick(), 48000.0);
    const auto& sample = *result.kit->slotForNote(36)->sample;
    ASSERT_EQ(sample.numChannels(), 2);
    EXPECT_FLOAT_EQ(sample.channels[0][0], 2.0F);
}

TEST(PitchToPlaybackRateTest, FSE07_SemitonesChangePlaybackSpeed)
{
    EXPECT_DOUBLE_EQ(pitchToPlaybackRate(0), 1.0);
    EXPECT_DOUBLE_EQ(pitchToPlaybackRate(12), 2.0);
    EXPECT_DOUBLE_EQ(pitchToPlaybackRate(-12), 0.5);
    EXPECT_NEAR(pitchToPlaybackRate(7), std::pow(2.0, 7.0 / 12.0), 1.0e-12);
}

TEST(PitchToPlaybackRateTest, FSE07_ClampsToTwelveSemitones)
{
    EXPECT_DOUBLE_EQ(pitchToPlaybackRate(24), 2.0);
    EXPECT_DOUBLE_EQ(pitchToPlaybackRate(-24), 0.5);
}

} // namespace
} // namespace drumprog::engine
