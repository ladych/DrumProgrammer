#include "engine/KitPublisher.h"

#include "engine/MockSampleLoader.h"
#include "engine/RenderHelpers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace drumprog::engine
{
namespace
{

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

const SampleBuffer kKick{48000.0, {std::vector<float>(64, 1.0F)}};

KitDescription kitWith(const std::filesystem::path& kickFile)
{
    return {KitSlotDescription{.gmNote = 36, .midiNote = 36, .name = "Kick", .sampleFile = kickFile}};
}

class KitPublisherTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(loader, load(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(loader, load(std::filesystem::path{"kick.wav"})).WillByDefault(Return(kKick));
        engine.prepare(48000.0);
    }

    float renderFrame()
    {
        StereoOutput out(1);
        engine.render(out.channels.data(), 2, 1);
        return out.left[0];
    }

    NiceMock<MockSampleLoader> loader;
    KitBuilder builder{loader};
    SampleEngine engine;
    KitPublisher publisher{builder, engine};
};

TEST_F(KitPublisherTest, FSE01_PublishedKitIsPlayedByTheEngine)
{
    publisher.publishKit(kitWith("kick.wav"));
    engine.queueTrigger(36, 127);

    EXPECT_GT(renderFrame(), 0.0F);
    EXPECT_TRUE(publisher.missingSamples().empty());
}

TEST_F(KitPublisherTest, FPJ03_ReportsMissingSamplesOfThePublishedKit)
{
    publisher.publishKit(kitWith("gone.wav"));

    ASSERT_EQ(publisher.missingSamples().size(), 1U);
    EXPECT_EQ(publisher.missingSamples()[0], std::filesystem::path{"gone.wav"});
}

TEST_F(KitPublisherTest, FSE02_RebuildsTheLastKitAtANewSampleRate)
{
    publisher.publishKit(kitWith("gone.wav"));
    EXPECT_CALL(loader, load(std::filesystem::path{"gone.wav"})).WillOnce(Return(std::nullopt));

    publisher.setDeviceSampleRate(44100.0);

    EXPECT_EQ(publisher.missingSamples().size(), 1U);
}

TEST_F(KitPublisherTest, FSE01_CanLoadChecksTheFileBeforeItIsAssigned)
{
    EXPECT_TRUE(publisher.canLoad("kick.wav"));
    EXPECT_FALSE(publisher.canLoad("gone.wav"));
}

} // namespace
} // namespace drumprog::engine
