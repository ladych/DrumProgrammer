#include "engine/DrumGizmoLoader.h"

#include "engine/MockDrumGizmoReader.h"
#include "engine/MockSampleLoader.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>
#include <new>

namespace drumprog::engine
{
namespace
{

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Throw;

constexpr double kRate = 48000.0;
const std::filesystem::path kDrumkit{"/kits/test/drumkit.xml"};

/// Four microphones in one file: OHL, OHR, kick close mic (centre) and a trigger track.
const SampleBuffer kFourMics{kRate, {{1.0F, 1.0F}, {2.0F, 2.0F}, {4.0F, 4.0F}, {8.0F, 8.0F}}};
const SampleBuffer kMonoLong{kRate, {{1.0F, 1.0F, 1.0F, 1.0F}}};

DrumGizmoSample hit(const std::string& name, double power, std::vector<DrumGizmoAudioFile> files)
{
    return {.name = name, .power = power, .audioFiles = std::move(files)};
}

DrumGizmoKit testKit()
{
    const std::filesystem::path multi{"/kits/test/kick-1.wav"};
    DrumGizmoKit kit;
    kit.instruments.push_back(
        {.name = "Kick",
         .group = "",
         .samples = {
             hit("hard",
                 8.0,
                 {{.channel = "OHL", .file = multi, .fileChannel = 1},
                  {.channel = "OHR", .file = multi, .fileChannel = 2},
                  {.channel = "Kdrum", .file = multi, .fileChannel = 3},
                  {.channel = "Trigger", .file = multi, .fileChannel = 4}}),
             hit("soft", 2.0, {{.channel = "Kdrum", .file = "/kits/test/kick-2.wav", .fileChannel = 1}}),
             hit("middle", 4.0, {{.channel = "Kdrum", .file = "/kits/test/kick-3.wav", .fileChannel = 1}})}});
    kit.instruments.push_back(
        {.name = "Snare",
         .group = "",
         .samples = {
             hit("only", 1.0, {{.channel = "Snare", .file = "/kits/test/gone.wav", .fileChannel = 1}})}});
    return kit;
}

class DrumGizmoLoaderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(loader, load(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(loader, load(std::filesystem::path{"/kits/test/kick-1.wav"}))
            .WillByDefault(Return(kFourMics));
        ON_CALL(loader, load(std::filesystem::path{"/kits/test/kick-2.wav"}))
            .WillByDefault(Return(kMonoLong));
        ON_CALL(loader, load(std::filesystem::path{"/kits/test/kick-3.wav"}))
            .WillByDefault(Return(kMonoLong));
    }

    NiceMock<MockSampleLoader> loader;
    DrumGizmoLoadProgress progress;
};

TEST_F(DrumGizmoLoaderTest, FDG03_MixesTheMicrophonesToStereoAndMutesTriggers)
{
    const auto loaded = loadDrumGizmoSamples(testKit(), loader, kRate, progress);

    const auto& kick = loaded.instruments.at("Kick");
    ASSERT_EQ(kick.size(), 3U);
    const auto& hard = *kick[2].sample;
    ASSERT_EQ(hard.numChannels(), 2);
    // Three active mics: 1/sqrt(3) each, the overheads sqrt(2) louder on their side; trigger (8) is silent.
    const float gain = 1.0F / std::sqrt(3.0F);
    EXPECT_FLOAT_EQ(hard.channels[0][0], gain * std::sqrt(2.0F) * 1.0F + gain * 4.0F);
    EXPECT_FLOAT_EQ(hard.channels[1][0], gain * std::sqrt(2.0F) * 2.0F + gain * 4.0F);
}

TEST_F(DrumGizmoLoaderTest, FDG02_HitsAreSortedByPowerFromSoftestToHardest)
{
    const auto loaded = loadDrumGizmoSamples(testKit(), loader, kRate, progress);

    const auto& kick = loaded.instruments.at("Kick");
    ASSERT_EQ(kick.size(), 3U);
    EXPECT_FLOAT_EQ(kick[0].power, 0.0F);
    EXPECT_FLOAT_EQ(kick[1].power, 1.0F / 3.0F);
    EXPECT_FLOAT_EQ(kick[2].power, 1.0F);
    EXPECT_EQ(kick[0].sample->numFrames(), 4);
}

TEST_F(DrumGizmoLoaderTest, Q09_MissingFilesAreReportedOnceAndTheirHitsLeftOut)
{
    auto kit = testKit();
    kit.instruments[1].samples.push_back(kit.instruments[1].samples[0]);

    const auto loaded = loadDrumGizmoSamples(kit, loader, kRate, progress);

    EXPECT_TRUE(loaded.instruments.at("Snare").empty());
    EXPECT_EQ(loaded.missingFiles, (std::vector<std::filesystem::path>{"/kits/test/gone.wav"}));
}

TEST_F(DrumGizmoLoaderTest, FDG01_EachFileOfAHitIsLoadedOnce)
{
    EXPECT_CALL(loader, load(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(loader, load(std::filesystem::path{"/kits/test/kick-1.wav"})).Times(1);

    static_cast<void>(loadDrumGizmoSamples(testKit(), loader, kRate, progress));
}

TEST_F(DrumGizmoLoaderTest, FDG01_ChannelsBeyondTheFileAndHitsWithoutFilesStaySilent)
{
    DrumGizmoKit kit;
    kit.instruments.push_back(
        {.name = "Tom",
         .group = "",
         .samples = {
             hit("empty", 1.0, {}),
             hit("beyond", 1.0, {{.channel = "Tom", .file = "/kits/test/kick-2.wav", .fileChannel = 2}})}});

    const auto loaded = loadDrumGizmoSamples(kit, loader, kRate, progress);

    EXPECT_TRUE(loaded.instruments.at("Tom").empty());
    EXPECT_TRUE(loaded.missingFiles.empty());
}

TEST_F(DrumGizmoLoaderTest, FDG02_HitsOfEqualPowerCountAsHardest)
{
    DrumGizmoKit kit;
    kit.instruments.push_back(
        {.name = "Ride",
         .group = "",
         .samples = {
             hit("a", 3.0, {{.channel = "Ride", .file = "/kits/test/kick-2.wav", .fileChannel = 1}}),
             hit("b", 3.0, {{.channel = "Ride", .file = "/kits/test/kick-3.wav", .fileChannel = 1}})}});

    const auto loaded = loadDrumGizmoSamples(kit, loader, kRate, progress);

    const auto& ride = loaded.instruments.at("Ride");
    ASSERT_EQ(ride.size(), 2U);
    EXPECT_FLOAT_EQ(ride[0].power, 1.0F);
    EXPECT_FLOAT_EQ(ride[1].power, 1.0F);
}

TEST_F(DrumGizmoLoaderTest, FSE02_HitsAreResampledToTheDeviceRate)
{
    const auto loaded = loadDrumGizmoSamples(testKit(), loader, kRate * 2.0, progress);

    const auto& soft = *loaded.instruments.at("Kick")[0].sample;
    EXPECT_DOUBLE_EQ(soft.sampleRate, kRate * 2.0);
    EXPECT_EQ(soft.numFrames(), 8);
}

TEST_F(DrumGizmoLoaderTest, FDG10_CountsTheRamOfAllHitsAndTheProgress)
{
    const auto loaded = loadDrumGizmoSamples(testKit(), loader, kRate, progress);

    // Stereo floats: 2 + 4 + 4 frames.
    EXPECT_EQ(loaded.bytes, (2U + 4U + 4U) * 2U * sizeof(float));
    EXPECT_EQ(progress.total, 4);
    EXPECT_EQ(progress.done, 4);
}

TEST_F(DrumGizmoLoaderTest, Q08_StopsWhenCancelled)
{
    progress.cancelled = true;

    const auto loaded = loadDrumGizmoSamples(testKit(), loader, kRate, progress);

    EXPECT_TRUE(loaded.instruments.empty());
    EXPECT_EQ(progress.done, 0);
}

class DrumGizmoBackgroundLoaderTest : public DrumGizmoLoaderTest
{
protected:
    void SetUp() override
    {
        DrumGizmoLoaderTest::SetUp();
        ON_CALL(reader, read(kDrumkit, true))
            .WillByDefault(Return(DrumGizmoReadResult{.kit = testKit(), .error = DrumGizmoError::none}));
    }

    NiceMock<MockDrumGizmoReader> reader;
    DrumGizmoBackgroundLoader background{reader, loader};
};

TEST_F(DrumGizmoBackgroundLoaderTest, Q08_LoadsOnItsOwnThreadAndHandsTheResultOverOnce)
{
    EXPECT_FALSE(background.isLoading());
    EXPECT_FLOAT_EQ(background.progress(), 0.0F);

    background.start(kDrumkit, kRate);
    background.waitUntilFinished();

    EXPECT_FALSE(background.isLoading());
    EXPECT_FLOAT_EQ(background.progress(), 1.0F);
    auto result = background.takeResult();
    ASSERT_TRUE(result);
    EXPECT_EQ(result->error, DrumGizmoError::none);
    ASSERT_NE(result->kit, nullptr);
    EXPECT_EQ(result->kit->instruments.size(), 2U);
    EXPECT_FALSE(background.takeResult());
}

TEST_F(DrumGizmoBackgroundLoaderTest, Q09_ReadErrorsAreHandedOver)
{
    ON_CALL(reader, read(kDrumkit, true))
        .WillByDefault(Return(DrumGizmoReadResult{.kit = std::nullopt,
                                                  .error = DrumGizmoError::instrumentMissing,
                                                  .errorFile = "/kits/test/x.xml"}));

    background.start(kDrumkit, kRate);
    background.waitUntilFinished();

    const auto result = background.takeResult();
    ASSERT_TRUE(result);
    EXPECT_EQ(result->error, DrumGizmoError::instrumentMissing);
    EXPECT_EQ(result->errorFile, std::filesystem::path{"/kits/test/x.xml"});
    EXPECT_EQ(result->kit, nullptr);
}

TEST_F(DrumGizmoBackgroundLoaderTest, Q09_TooLittleMemoryIsAnErrorNotACrash)
{
    ON_CALL(loader, load(_)).WillByDefault(Throw(std::bad_alloc{}));

    background.start(kDrumkit, kRate);
    background.waitUntilFinished();

    const auto result = background.takeResult();
    ASSERT_TRUE(result);
    EXPECT_EQ(result->error, DrumGizmoError::outOfMemory);
    EXPECT_EQ(result->errorFile, kDrumkit);
    EXPECT_EQ(result->kit, nullptr);
}

TEST_F(DrumGizmoBackgroundLoaderTest, Q08_CancelDropsTheResultAndANewStartReplacesTheOldLoad)
{
    background.start(kDrumkit, kRate);
    background.cancel();
    EXPECT_FALSE(background.takeResult());

    background.start(kDrumkit, kRate);
    background.start(kDrumkit, kRate * 2.0);
    background.waitUntilFinished();

    const auto result = background.takeResult();
    ASSERT_TRUE(result);
    ASSERT_NE(result->kit, nullptr);
    EXPECT_DOUBLE_EQ(result->kit->instruments.at("Kick")[0].sample->sampleRate, kRate * 2.0);
}

} // namespace
} // namespace drumprog::engine
