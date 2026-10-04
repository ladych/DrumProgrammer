#include "engine/VoicePool.h"

#include "engine/RenderHelpers.h"

#include <gtest/gtest.h>

namespace drumprog::engine
{
namespace
{

const SampleBuffer kLong{48000.0, {std::vector<float>(48000, 1.0F)}};
const SampleBuffer kShort{48000.0, {{1.0F, 1.0F}}};

class VoicePoolTest : public ::testing::Test
{
protected:
    void SetUp() override { pool.prepare(48000.0); }

    float renderOneFrame()
    {
        StereoOutput out(1);
        pool.render(out.channels.data(), 2, 1);
        return out.left[0];
    }

    VoicePool pool;
};

TEST_F(VoicePoolTest, FSE08_FadeIsFiveMilliseconds)
{
    EXPECT_EQ(pool.fadeSamples(), 240);
    pool.prepare(44100.0);
    EXPECT_EQ(pool.fadeSamples(), 221);
}

TEST_F(VoicePoolTest, FSE08_FadeIsAtLeastOneSample)
{
    pool.prepare(0.0);
    EXPECT_EQ(pool.fadeSamples(), 1);
}

TEST_F(VoicePoolTest, FSE03_PlaysOverlappingNotesPolyphonically)
{
    pool.trigger({.sample = &kLong, .gain = 0.25F});
    pool.trigger({.sample = &kLong, .gain = 0.5F});
    EXPECT_EQ(pool.playingVoiceCount(), 2);
    EXPECT_FLOAT_EQ(renderOneFrame(), 0.75F);
}

TEST_F(VoicePoolTest, FSE03_PlaysSixtyFourVoicesAtOnce)
{
    for (int i = 0; i < VoicePool::kMaxVoices; ++i)
        pool.trigger({.sample = &kLong, .gain = 1.0F});
    EXPECT_EQ(pool.playingVoiceCount(), 64);
    EXPECT_EQ(pool.soundingVoiceCount(), 64);
    EXPECT_FLOAT_EQ(renderOneFrame(), 64.0F);
}

TEST_F(VoicePoolTest, FSE03_OverflowFadesOutOldestVoice)
{
    int oldest = 0;
    pool.trigger({.sample = &kLong, .owner = &oldest});
    for (int i = 1; i < VoicePool::kMaxVoices; ++i)
        pool.trigger({.sample = &kLong});
    pool.trigger({.sample = &kLong});
    EXPECT_EQ(pool.playingVoiceCount(), 64);
    EXPECT_EQ(pool.soundingVoiceCount(), 65);
    EXPECT_TRUE(pool.isOwnerInUse(&oldest));
    StereoOutput out(pool.fadeSamples());
    pool.render(out.channels.data(), 2, out.frames());
    EXPECT_FALSE(pool.isOwnerInUse(&oldest));
    EXPECT_EQ(pool.soundingVoiceCount(), 64);
}

TEST_F(VoicePoolTest, FSE03_WhenReserveIsExhaustedOldestFadingVoiceIsCut)
{
    for (int i = 0; i < VoicePool::kMaxVoices + VoicePool::kReserveVoices + 5; ++i)
        pool.trigger({.sample = &kLong});
    EXPECT_EQ(pool.playingVoiceCount(), 64);
    EXPECT_EQ(pool.soundingVoiceCount(), 72);
}

TEST_F(VoicePoolTest, FSE03_FinishedVoicesAreFreed)
{
    pool.trigger({.sample = &kShort});
    StereoOutput out(4);
    pool.render(out.channels.data(), 2, 4);
    EXPECT_EQ(pool.soundingVoiceCount(), 0);
}

TEST_F(VoicePoolTest, FSE08_ChokeGroupFadesOutOtherVoicesOfGroup)
{
    pool.trigger({.sample = &kLong, .chokeGroup = 1});
    pool.trigger({.sample = &kLong, .chokeGroup = 2});
    pool.trigger({.sample = &kLong, .chokeGroup = 0});
    pool.trigger({.sample = &kLong, .chokeGroup = 1});
    EXPECT_EQ(pool.playingVoiceCount(), 3);
    StereoOutput out(pool.fadeSamples());
    pool.render(out.channels.data(), 2, out.frames());
    EXPECT_EQ(pool.soundingVoiceCount(), 3);
}

TEST_F(VoicePoolTest, FSE08_ChokeWithoutGroupDoesNothing)
{
    pool.trigger({.sample = &kLong, .chokeGroup = 0});
    pool.choke(0);
    EXPECT_EQ(pool.playingVoiceCount(), 1);
}

TEST_F(VoicePoolTest, FSE03_StopAllSilencesEverything)
{
    pool.trigger({.sample = &kLong});
    pool.trigger({.sample = &kLong});
    pool.stopAll();
    EXPECT_EQ(pool.soundingVoiceCount(), 0);
    EXPECT_FLOAT_EQ(renderOneFrame(), 0.0F);
}

TEST_F(VoicePoolTest, FSE03_IgnoresTriggerWithoutSample)
{
    pool.trigger({.sample = nullptr});
    EXPECT_EQ(pool.soundingVoiceCount(), 0);
}

} // namespace
} // namespace drumprog::engine
