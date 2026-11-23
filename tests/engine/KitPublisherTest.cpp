#include "engine/KitPublisher.h"

#include "engine/MockDrumGizmoReader.h"
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

const std::filesystem::path kDrumkit{"/dg/drumkit.xml"};

KitDescription drumGizmoKit()
{
    return {KitSlotDescription{
        .gmNote = 36, .midiNote = 36, .name = "Kick", .drumkitFile = kDrumkit, .instrument = "Kdrum"}};
}

DrumGizmoKit kdrumKit()
{
    DrumGizmoKit kit;
    kit.instruments.push_back(
        {.name = "Kdrum",
         .group = "",
         .samples = {{.name = "1",
                      .power = 1.0,
                      .audioFiles = {{.channel = "Kdrum", .file = "kick.wav", .fileChannel = 1},
                                     {.channel = "OHL", .file = "gone.wav", .fileChannel = 1}}}}});
    return kit;
}

class KitPublisherDrumGizmoTest : public KitPublisherTest
{
protected:
    void SetUp() override
    {
        KitPublisherTest::SetUp();
        ON_CALL(reader, read(kDrumkit, true)).WillByDefault(Return(DrumGizmoReadResult{.kit = kdrumKit()}));
    }

    /// Lets the background load finish and publishes it.
    bool finishLoading()
    {
        background.waitUntilFinished();
        return dgPublisher.poll();
    }

    NiceMock<MockDrumGizmoReader> reader;
    DrumGizmoBackgroundLoader background{reader, loader};
    KitPublisher dgPublisher{builder, engine, &background};
};

TEST_F(KitPublisherDrumGizmoTest, Q08_DrumGizmoKitLoadsInTheBackgroundAndPlaysOnceReady)
{
    dgPublisher.publishKit(drumGizmoKit());
    EXPECT_EQ(dgPublisher.drumGizmoStatus().state, DrumGizmoStatus::State::loading);
    engine.queueTrigger(36, 127);
    EXPECT_FLOAT_EQ(renderFrame(), 0.0F);

    EXPECT_TRUE(finishLoading());

    const auto& status = dgPublisher.drumGizmoStatus();
    EXPECT_EQ(status.state, DrumGizmoStatus::State::ready);
    EXPECT_EQ(status.bytes, 64U * 2U * sizeof(float));
    EXPECT_EQ(status.missingFiles, 1U);
    EXPECT_EQ(dgPublisher.missingSamples(), (std::vector<std::filesystem::path>{"gone.wav"}));
    EXPECT_FLOAT_EQ(dgPublisher.drumGizmoProgress(), 1.0F);
    engine.queueTrigger(36, 127);
    EXPECT_GT(renderFrame(), 0.0F);
    EXPECT_FALSE(dgPublisher.poll());
}

TEST_F(KitPublisherDrumGizmoTest, Q09_AKitThatCannotBeReadFails)
{
    ON_CALL(reader, read(kDrumkit, true))
        .WillByDefault(Return(DrumGizmoReadResult{
            .kit = std::nullopt, .error = DrumGizmoError::notADrumkit, .errorFile = kDrumkit}));

    dgPublisher.publishKit(drumGizmoKit());
    EXPECT_TRUE(finishLoading());

    const auto& status = dgPublisher.drumGizmoStatus();
    EXPECT_EQ(status.state, DrumGizmoStatus::State::failed);
    EXPECT_EQ(status.error, DrumGizmoError::notADrumkit);
    EXPECT_EQ(status.errorFile, kDrumkit);
    EXPECT_EQ(status.bytes, 0U);
    EXPECT_EQ(status.missingFiles, 0U);
}

TEST_F(KitPublisherDrumGizmoTest, Q05_EditsOfTheSameKitDoNotLoadItAgain)
{
    EXPECT_CALL(reader, read(kDrumkit, true)).Times(1);

    dgPublisher.publishKit(drumGizmoKit());
    finishLoading();
    auto louder = drumGizmoKit();
    louder[0].gain = 2.0F;
    dgPublisher.publishKit(louder);

    EXPECT_EQ(dgPublisher.drumGizmoStatus().state, DrumGizmoStatus::State::ready);
}

TEST_F(KitPublisherDrumGizmoTest, FSE02_ANewSampleRateOrAReloadLoadsTheKitAgain)
{
    EXPECT_CALL(reader, read(kDrumkit, true)).Times(3);

    dgPublisher.publishKit(drumGizmoKit());
    finishLoading();
    dgPublisher.setDeviceSampleRate(44100.0);
    EXPECT_EQ(dgPublisher.drumGizmoStatus().state, DrumGizmoStatus::State::loading);
    finishLoading();
    dgPublisher.reloadDrumGizmoKit();
    EXPECT_EQ(dgPublisher.drumGizmoStatus().state, DrumGizmoStatus::State::loading);
    finishLoading();
}

TEST_F(KitPublisherDrumGizmoTest, Q05_AKitWithoutSlotsWhileTheKitIsReplacedKeepsTheLoadedKit)
{
    EXPECT_CALL(reader, read(kDrumkit, true)).Times(1);

    dgPublisher.publishKit(drumGizmoKit());
    finishLoading();
    dgPublisher.publishKit({});
    dgPublisher.publishKit(drumGizmoKit());

    EXPECT_EQ(dgPublisher.drumGizmoStatus().state, DrumGizmoStatus::State::ready);
}

TEST_F(KitPublisherDrumGizmoTest, FDG04_ASampleKitEndsTheDrumGizmoKit)
{
    dgPublisher.publishKit(drumGizmoKit());
    dgPublisher.publishKit(kitWith("kick.wav"));

    EXPECT_EQ(dgPublisher.drumGizmoStatus().state, DrumGizmoStatus::State::none);
    EXPECT_FALSE(finishLoading());
    dgPublisher.setDeviceSampleRate(44100.0);
    EXPECT_EQ(dgPublisher.drumGizmoStatus().state, DrumGizmoStatus::State::none);
}

TEST_F(KitPublisherTest, FDG04_WithoutALoaderDrumGizmoSlotsStaySilent)
{
    publisher.publishKit(drumGizmoKit());

    EXPECT_EQ(publisher.drumGizmoStatus().state, DrumGizmoStatus::State::none);
    EXPECT_FALSE(publisher.poll());
    EXPECT_FLOAT_EQ(publisher.drumGizmoProgress(), 0.0F);
    engine.queueTrigger(36, 127);
    EXPECT_FLOAT_EQ(renderFrame(), 0.0F);
}

} // namespace
} // namespace drumprog::engine
