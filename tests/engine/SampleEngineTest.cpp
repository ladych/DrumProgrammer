#include "engine/SampleEngine.h"

#include "engine/RenderHelpers.h"
#include "engine/VelocityCurve.h"

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <thread>

namespace drumprog::engine
{
namespace
{

constexpr double kRate = 48000.0;

std::shared_ptr<const SampleBuffer> constantSample(float value, int frames)
{
    return std::make_shared<const SampleBuffer>(
        SampleBuffer{kRate, {std::vector<float>(static_cast<size_t>(frames), value)}});
}

std::unique_ptr<EngineKit> makeKit(std::shared_ptr<const SampleBuffer> sample, float gain = 1.0F,
                                   int chokeGroup = 0)
{
    return std::make_unique<EngineKit>(std::vector<EngineSlot>{
        EngineSlot{.slotIndex = 0, .midiNote = 36, .sample = std::move(sample), .gain = gain},
        EngineSlot{.slotIndex = 1, .midiNote = 38},
        EngineSlot{.slotIndex = 2, .midiNote = 42, .sample = constantSample(1.0F, 1000), .chokeGroup = chokeGroup},
        EngineSlot{.slotIndex = 3, .midiNote = 46, .sample = constantSample(1.0F, 1000), .chokeGroup = chokeGroup}});
}

class SampleEngineTest : public ::testing::Test
{
protected:
    void SetUp() override { engine.prepare(kRate); }

    std::vector<float> render(int frames)
    {
        StereoOutput out(frames);
        engine.render(out.channels.data(), 2, frames);
        return out.left;
    }

    SampleEngine engine;
};

TEST_F(SampleEngineTest, FSE03_RendersSilenceWithoutKit)
{
    engine.trigger(36, 127, 0);
    EXPECT_TRUE(engine.queueTrigger(36, 127));
    EXPECT_EQ(render(4), std::vector<float>(4, 0.0F));
}

TEST_F(SampleEngineTest, FSE06_QueuedTriggerPlaysSlotWithGainAndVelocity)
{
    engine.setKit(makeKit(constantSample(1.0F, 100), 0.5F));
    EXPECT_TRUE(engine.queueTrigger(36, 127));
    const auto out = render(2);
    EXPECT_FLOAT_EQ(out[0], 0.5F);
    EXPECT_FLOAT_EQ(out[1], 0.5F);
}

TEST_F(SampleEngineTest, FTR06_DirectTriggerStartsAtSampleOffset)
{
    engine.setKit(makeKit(constantSample(1.0F, 100)));
    render(1);
    engine.trigger(36, 127, 2);
    EXPECT_EQ(render(4), (std::vector<float>{0.0F, 0.0F, 1.0F, 1.0F}));
}

TEST_F(SampleEngineTest, FSE10_PreviewPlaysWithVelocity100)
{
    engine.setKit(makeKit(constantSample(1.0F, 100)));
    EXPECT_TRUE(engine.preview(36));
    const auto out = render(1);
    EXPECT_FLOAT_EQ(out[0], velocityToGain(SampleEngine::kPreviewVelocity));
}

TEST_F(SampleEngineTest, FSE09_TriggerLightsLedEvenWithoutSample)
{
    engine.setKit(makeKit(constantSample(1.0F, 100)));
    engine.queueTrigger(38, 100);
    engine.queueTrigger(36, 100);
    EXPECT_EQ(render(1)[0] > 0.0F, true);
    EXPECT_EQ(engine.indicators().hitCount(0), 1U);
    EXPECT_EQ(engine.indicators().hitCount(1), 1U);
}

TEST_F(SampleEngineTest, FIN04_VelocityZeroAndUnknownNotesAreIgnored)
{
    engine.setKit(makeKit(constantSample(1.0F, 100)));
    render(1);
    engine.trigger(36, 0, 0);
    engine.trigger(60, 100, 0);
    EXPECT_EQ(render(2), std::vector<float>(2, 0.0F));
    EXPECT_EQ(engine.indicators().hitCount(0), 0U);
}

TEST_F(SampleEngineTest, FSE08_ClosedHiHatChokesOpenHiHat)
{
    engine.setKit(makeKit(constantSample(1.0F, 100), 1.0F, 1));
    engine.queueTrigger(46, 127);
    render(10);
    engine.queueTrigger(42, 127);
    // Open hi-hat fades over 5 ms while the closed hi-hat plays.
    const auto out = render(241);
    EXPECT_NEAR(out[0], 2.0F, 1.0e-5);
    EXPECT_NEAR(out[240], 1.0F, 1.0e-5);
}

TEST_F(SampleEngineTest, FSE03_QueueRejectsTriggersWhenFull)
{
    for (std::size_t i = 0; i < SampleEngine::kTriggerQueueSize; ++i)
        ASSERT_TRUE(engine.queueTrigger(36, 100));
    EXPECT_FALSE(engine.queueTrigger(36, 100));
}

TEST_F(SampleEngineTest, Q04_NewKitIsUsedFromNextBlock)
{
    engine.setKit(makeKit(constantSample(1.0F, 100)));
    engine.queueTrigger(36, 127);
    EXPECT_FLOAT_EQ(render(1)[0], 1.0F);
    engine.setKit(makeKit(constantSample(0.25F, 100)));
    engine.queueTrigger(36, 127);
    EXPECT_FLOAT_EQ(render(1)[0], 1.25F);
}

TEST_F(SampleEngineTest, Q04_ReplacedKitIsKeptWhileItsVoicesPlay)
{
    auto sample = constantSample(1.0F, 10);
    const std::weak_ptr<const SampleBuffer> watch = sample;
    engine.setKit(makeKit(std::move(sample)));
    engine.queueTrigger(36, 127);
    render(1);
    engine.setKit(makeKit(constantSample(0.5F, 10)));
    render(1);
    engine.collectGarbage();
    EXPECT_FALSE(watch.expired());
    render(20);
    engine.collectGarbage();
    EXPECT_TRUE(watch.expired());
}

TEST_F(SampleEngineTest, Q04_UnusedReplacedKitIsFreedAfterNextBlock)
{
    auto sample = constantSample(1.0F, 10);
    const std::weak_ptr<const SampleBuffer> watch = sample;
    engine.setKit(makeKit(std::move(sample)));
    render(1);
    engine.setKit(makeKit(constantSample(0.5F, 10)));
    render(1);
    engine.collectGarbage();
    EXPECT_TRUE(watch.expired());
}

TEST_F(SampleEngineTest, Q04_KitPublishedTwiceBeforeBlockReplacesPendingKit)
{
    auto sample = constantSample(1.0F, 10);
    const std::weak_ptr<const SampleBuffer> watch = sample;
    engine.setKit(makeKit(std::move(sample)));
    engine.setKit(makeKit(constantSample(0.5F, 10)));
    EXPECT_TRUE(watch.expired());
    engine.queueTrigger(36, 127);
    EXPECT_FLOAT_EQ(render(1)[0], 0.5F);
}

TEST_F(SampleEngineTest, Q04_DrainingKitDelaysNextKitUntilVoicesEnd)
{
    engine.setKit(makeKit(constantSample(1.0F, 10)));
    engine.queueTrigger(36, 127);
    render(1);
    engine.setKit(makeKit(constantSample(0.5F, 10))); // first kit drains
    render(1);
    engine.setKit(makeKit(constantSample(0.25F, 10)));
    engine.queueTrigger(36, 127);
    EXPECT_FLOAT_EQ(render(1)[0], 1.5F); // still second kit, first voice still sounding
    render(10);
    engine.collectGarbage();
    engine.queueTrigger(36, 127);
    EXPECT_FLOAT_EQ(render(1)[0], 0.25F);
}

TEST_F(SampleEngineTest, Q04_ReplacedKitWaitsUntilPreviousGarbageIsCollected)
{
    auto first = constantSample(1.0F, 10);
    auto second = constantSample(0.5F, 10);
    const std::weak_ptr<const SampleBuffer> watchFirst = first;
    const std::weak_ptr<const SampleBuffer> watchSecond = second;
    engine.setKit(makeKit(std::move(first)));
    render(1);
    engine.setKit(makeKit(std::move(second)));
    render(1); // first kit retired
    engine.setKit(makeKit(constantSample(0.25F, 10)));
    render(1); // second kit unused, but the retire slot is still taken
    engine.collectGarbage();
    EXPECT_TRUE(watchFirst.expired());
    EXPECT_FALSE(watchSecond.expired());
    render(1);
    engine.collectGarbage();
    EXPECT_TRUE(watchSecond.expired());
    engine.queueTrigger(36, 127);
    EXPECT_FLOAT_EQ(render(1)[0], 0.25F);
}

TEST_F(SampleEngineTest, Q04_PublishingKitsWhileRenderingIsThreadSafe)
{
    std::atomic<bool> running{true};
    std::thread audio([this, &running] {
        StereoOutput out(64);
        while (running.load())
        {
            engine.trigger(36, 100, 0);
            engine.render(out.channels.data(), 2, 64);
        }
    });
    for (int i = 0; i < 200; ++i)
    {
        engine.setKit(makeKit(constantSample(0.1F, 200)));
        engine.queueTrigger(42, 100);
        engine.collectGarbage();
        std::this_thread::yield();
    }
    running.store(false);
    audio.join();
    engine.collectGarbage();
    SUCCEED();
}

} // namespace
} // namespace drumprog::engine
