#include "engine/BackingTrackPlayer.h"

#include "engine/FakeAudioFileStream.h"
#include "engine/RenderHelpers.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace drumprog::engine
{
namespace
{

constexpr double kRate = 48000.0;

class BackingTrackPlayerTest : public ::testing::Test
{
protected:
    void SetUp() override { player.prepare(kRate); }

    FakeAudioFileStream& load(std::unique_ptr<FakeAudioFileStream> stream)
    {
        auto& result = *stream;
        player.setStream(std::move(stream));
        player.update();
        return result;
    }

    StereoOutput render(int frames, std::int64_t songSample, std::int64_t offset = 0, float gain = 1.0F)
    {
        StereoOutput out(frames);
        player.render(out.channels.data(), 2, frames, songSample, offset, gain);
        return out;
    }

    BackingTrackPlayer player;
};

TEST_F(BackingTrackPlayerTest, FBT01_WithoutTrackItPlaysNothing)
{
    const auto out = render(16, 0);
    EXPECT_EQ(out.left, std::vector<float>(16, 0.0F));
    EXPECT_EQ(player.endSample(0), 0);
}

TEST_F(BackingTrackPlayerTest, FBT03_PlaysTheFrameOfTheSongPositionAtTheSameRate)
{
    load(FakeAudioFileStream::ramp(kRate, 1000));
    const auto out = render(4, 100, 0, 0.5F);
    EXPECT_EQ(out.left, (std::vector<float>{50.5F, 51.0F, 51.5F, 52.0F}));
    EXPECT_EQ(out.right, (std::vector<float>{-50.5F, -51.0F, -51.5F, -52.0F}));
}

TEST_F(BackingTrackPlayerTest, FBT03_AddsToTheOutputs)
{
    load(FakeAudioFileStream::ramp(kRate, 1000));
    StereoOutput out(2);
    out.left = {1.0F, 1.0F};
    player.render(out.channels.data(), 2, 2, 0, 0, 1.0F);
    EXPECT_EQ(out.left, (std::vector<float>{2.0F, 3.0F}));
}

TEST_F(BackingTrackPlayerTest, FBT05_OffsetIsTheFilePositionAtTheSongStart)
{
    load(FakeAudioFileStream::ramp(kRate, 1000));
    EXPECT_EQ(render(1, 0, 200).left[0], 201.0F); // lead-in skipped
    EXPECT_EQ(render(1, 0, -10).left[0], 0.0F);   // starts later
    EXPECT_EQ(render(1, 10, -10).left[0], 1.0F);
    EXPECT_EQ(render(1, 1000).left[0], 0.0F); // after the end
}

TEST_F(BackingTrackPlayerTest, FBT02_ResamplesToTheDeviceRate)
{
    // 24 kHz file on 48 kHz: every file frame lasts two device samples, in between interpolated.
    load(FakeAudioFileStream::ramp(kRate / 2, 1000));
    const auto out = render(4, 200);
    EXPECT_FLOAT_EQ(out.left[0], 101.0F);
    EXPECT_FLOAT_EQ(out.left[1], 101.5F);
    EXPECT_FLOAT_EQ(out.left[2], 102.0F);
    EXPECT_FLOAT_EQ(out.left[3], 102.5F);
}

TEST_F(BackingTrackPlayerTest, FBT02_InterpolationIsSmoothForASine)
{
    std::vector<float> sine;
    for (int frame = 0; frame < 48000; ++frame)
        sine.push_back(static_cast<float>(std::sin(frame * 0.01)));
    load(std::make_unique<FakeAudioFileStream>(44100.0, sine, sine));
    const auto out = render(2000, 3000);
    for (int index = 0; index < out.frames(); ++index)
    {
        const double filePosition = (3000.0 + index) * 44100.0 / kRate;
        EXPECT_NEAR(out.left[static_cast<std::size_t>(index)], std::sin(filePosition * 0.01), 1e-4);
    }
}

TEST_F(BackingTrackPlayerTest, FBT02_Q04_LongBlocksAreReadInBoundedSteps)
{
    // 96 kHz file on 48 kHz: two file frames per device sample.
    auto& stream = load(FakeAudioFileStream::ramp(2 * kRate, 40000));
    const auto out = render(8000, 0);
    EXPECT_LE(stream.largestRead, BackingTrackPlayer::kReadFrames);
    for (int index = 0; index < out.frames(); ++index)
        ASSERT_FLOAT_EQ(out.left[static_cast<std::size_t>(index)], static_cast<float>(2 * index + 1));
}

TEST_F(BackingTrackPlayerTest, FBT03_Q06_BlockSizeDoesNotChangeTheSignal)
{
    load(FakeAudioFileStream::ramp(44100.0, 20000));
    const auto whole = render(3000, 500);
    std::vector<float> pieces;
    for (int start = 0; start < 3000; start += 128)
    {
        const int size = std::min(128, 3000 - start);
        const auto part = render(size, 500 + start);
        pieces.insert(pieces.end(), part.left.begin(), part.left.end());
    }
    ASSERT_EQ(pieces.size(), whole.left.size());
    for (std::size_t index = 0; index < pieces.size(); ++index)
        EXPECT_FLOAT_EQ(pieces[index], whole.left[index]);
}

TEST_F(BackingTrackPlayerTest, FBT02_MonoOutputGetsBothChannels)
{
    load(std::make_unique<FakeAudioFileStream>(kRate, std::vector<float>{1.0F}, std::vector<float>{3.0F}));
    std::vector<float> mono(1, 0.0F);
    std::array<float*, 1> channels{mono.data()};
    player.render(channels.data(), 1, 1, 0, 0, 1.0F);
    EXPECT_FLOAT_EQ(mono[0], 2.0F);
    player.render(channels.data(), 0, 1, 0, 0, 1.0F);
    EXPECT_FLOAT_EQ(mono[0], 2.0F);
}

TEST_F(BackingTrackPlayerTest, FBT02_ExtremeRatesAreNotPlayed)
{
    load(FakeAudioFileStream::ramp(kRate * 100, 1000));
    EXPECT_EQ(render(1, 0).left[0], 0.0F);
}

TEST_F(BackingTrackPlayerTest, FBT03_EndSampleIsTheTrackEndInDeviceSamples)
{
    load(FakeAudioFileStream::ramp(kRate / 2, 1001));
    EXPECT_EQ(player.endSample(0), 2002);
    EXPECT_EQ(player.endSample(1), 2000);
    EXPECT_EQ(player.endSample(-10), 2022);
    EXPECT_EQ(player.endSample(5000), 0);
}

TEST_F(BackingTrackPlayerTest, FBT01_Q04_ANewTrackReplacesTheOldOneAfterGarbageCollection)
{
    load(FakeAudioFileStream::ramp(kRate, 100));
    player.setStream(FakeAudioFileStream::ramp(kRate, 200));
    player.setStream(FakeAudioFileStream::ramp(kRate, 300)); // replaces the one not picked up yet
    player.update();
    EXPECT_EQ(player.endSample(0), 300);

    player.setStream(nullptr);
    player.update(); // the first track still waits in the retire slot
    EXPECT_EQ(player.endSample(0), 300);
    player.collectGarbage();
    player.update();
    EXPECT_EQ(player.endSample(0), 0);
    EXPECT_EQ(render(1, 0).left[0], 0.0F);
}

TEST_F(BackingTrackPlayerTest, Q04_TheDestructorFreesEveryTrack)
{
    auto other = std::make_unique<BackingTrackPlayer>();
    other->setStream(FakeAudioFileStream::ramp(kRate, 10));
    other->update();
    other->setStream(FakeAudioFileStream::ramp(kRate, 10));
    other->update();
    other->setStream(FakeAudioFileStream::ramp(kRate, 10));
    other.reset();
    SUCCEED();
}

} // namespace
} // namespace drumprog::engine
