#include "io/DrumGizmoReader.h"

#include "io/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace drumprog::io
{
namespace
{

using engine::DrumGizmoError;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace fs = std::filesystem;

const fs::path kKitDir{"/kits/Aasimonster"};
const fs::path kDrumkit = kKitDir / "drumkit.xml";

// Shortened from the Aasimonster kit (format 2.0): channel map, power and file channels.
const std::string kDrumkitXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<drumkit version="2.0" name="Aasimonster" samplerate="44100">
  <channels>
    <channel name="AmbL"/>
    <channel name="OHR"/>
    <channel name="Kdrum_back"/>
    <channel name="Snare_trigger"/>
  </channels>
  <instruments>
    <instrument name="Kdrum" file="Kdrum/Kdrum.xml">
      <channelmap in="AmbLeft" out="AmbL"/>
      <channelmap in="OHR" out="OHR"/>
    </instrument>
    <instrument name="Hihat" group="hihat" file="Hihat/Hihat.xml"/>
  </instruments>
</drumkit>)";

const std::string kKickXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<instrument version="2.0" name="Kdrum">
  <samples>
    <sample name="Kdrum-1" power="0.25">
      <audiofile channel="AmbLeft" file="samples/1-Kdrum.wav" filechannel="1"/>
      <audiofile channel="OHR" file="samples/1-Kdrum.wav" filechannel="2"/>
    </sample>
    <sample name="Kdrum-2" power="3.5">
      <audiofile channel="Kdrum_back" file="../shared/2-Kdrum.wav"/>
    </sample>
  </samples>
</instrument>)";

// Format 1.0: no power attribute, velocity ranges refer to the samples.
const std::string kHiHatXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<instrument name="Hihat">
  <samples>
    <sample name="soft"><audiofile channel="OHR" file="hh-soft.wav" filechannel="0"/></sample>
    <sample name="hard"><audiofile channel="OHR" file="hh-hard.wav" filechannel="1"/></sample>
    <sample name="lost"/>
  </samples>
  <velocities>
    <velocity lower="0" upper="0.4"><sampleref name="soft" probability="1"/></velocity>
    <velocity lower="0.6" upper="1"><sampleref name="hard" probability="1"/></velocity>
  </velocities>
</instrument>)";

const std::string kMidiMapXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<midimap>
  <map note="36" instr="Kdrum"/>
  <map note="36" instr="Hihat"/>
  <map note="42" instr="Hihat"/>
</midimap>)";

class DrumGizmoReaderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(fileSystem, readText(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(fileSystem, exists(_)).WillByDefault(Return(false));
        ON_CALL(fileSystem, listFiles(_)).WillByDefault(Return(std::vector<fs::path>{}));
        file(kDrumkit, kDrumkitXml);
        file(kKitDir / "Kdrum/Kdrum.xml", kKickXml);
        file(kKitDir / "Hihat/Hihat.xml", kHiHatXml);
    }

    void file(const fs::path& path, const std::string& text)
    {
        ON_CALL(fileSystem, readText(path)).WillByDefault(Return(text));
        ON_CALL(fileSystem, exists(path)).WillByDefault(Return(true));
    }

    NiceMock<MockFileSystem> fileSystem;
    DrumGizmoReader reader{fileSystem};
};

TEST_F(DrumGizmoReaderTest, FDG01_ReadsNameChannelsAndInstruments)
{
    const auto result = reader.read(kDrumkit, true);

    ASSERT_TRUE(result.kit);
    EXPECT_EQ(result.error, DrumGizmoError::none);
    EXPECT_EQ(result.kit->name, "Aasimonster");
    EXPECT_EQ(result.kit->channels, (std::vector<std::string>{"AmbL", "OHR", "Kdrum_back", "Snare_trigger"}));
    ASSERT_EQ(result.kit->instruments.size(), 2U);
    EXPECT_EQ(result.kit->instruments[0].name, "Kdrum");
    EXPECT_EQ(result.kit->instruments[0].group, "");
    EXPECT_EQ(result.kit->instruments[1].group, "hihat");
}

TEST_F(DrumGizmoReaderTest, FDG01_ReadsSamplesWithPowerAndMappedChannels)
{
    const auto result = reader.read(kDrumkit, true);

    ASSERT_TRUE(result.kit);
    const auto& samples = result.kit->instruments[0].samples;
    ASSERT_EQ(samples.size(), 2U);
    EXPECT_EQ(samples[0].name, "Kdrum-1");
    EXPECT_DOUBLE_EQ(samples[0].power, 0.25);
    ASSERT_EQ(samples[0].audioFiles.size(), 2U);
    EXPECT_EQ(samples[0].audioFiles[0].channel, "AmbL");
    EXPECT_EQ(samples[0].audioFiles[0].file, kKitDir / "Kdrum/samples/1-Kdrum.wav");
    EXPECT_EQ(samples[0].audioFiles[0].fileChannel, 1);
    EXPECT_EQ(samples[0].audioFiles[1].channel, "OHR");
    EXPECT_EQ(samples[0].audioFiles[1].fileChannel, 2);
    // Without a channel map entry the instrument's channel name is the kit channel; paths are normalised.
    EXPECT_EQ(samples[1].audioFiles[0].channel, "Kdrum_back");
    EXPECT_EQ(samples[1].audioFiles[0].file, kKitDir / "shared/2-Kdrum.wav");
    EXPECT_EQ(samples[1].audioFiles[0].fileChannel, 1);
}

TEST_F(DrumGizmoReaderTest, FDG01_FormatOneTakesThePowerFromTheVelocityRanges)
{
    const auto result = reader.read(kDrumkit, true);

    ASSERT_TRUE(result.kit);
    const auto& samples = result.kit->instruments[1].samples;
    ASSERT_EQ(samples.size(), 3U);
    EXPECT_DOUBLE_EQ(samples[0].power, 0.2);
    EXPECT_DOUBLE_EQ(samples[1].power, 0.8);
    EXPECT_DOUBLE_EQ(samples[2].power, 0.0);
    EXPECT_TRUE(samples[2].audioFiles.empty());
    EXPECT_EQ(samples[0].audioFiles[0].fileChannel, 1);
}

TEST_F(DrumGizmoReaderTest, FDG01_WithoutSamplesOnlyTheInstrumentsAreRead)
{
    const auto result = reader.read(kDrumkit, false);

    ASSERT_TRUE(result.kit);
    ASSERT_EQ(result.kit->instruments.size(), 2U);
    EXPECT_TRUE(result.kit->instruments[0].samples.empty());
}

TEST_F(DrumGizmoReaderTest, FDG07_ReadsTheMidiMapNextToTheKitFirstEntryPerNoteWins)
{
    ON_CALL(fileSystem, listFiles(kKitDir))
        .WillByDefault(Return(std::vector<fs::path>{kDrumkit,
                                                    kKitDir / "Midimap_broken.xml",
                                                    kKitDir / "Midimap_other.txt",
                                                    kKitDir / "Midimap_wrong.xml",
                                                    kKitDir / "Midimap_full.xml",
                                                    kKitDir / "Midimap_second.xml"}));
    file(kKitDir / "Midimap_broken.xml", "no xml");
    file(kKitDir / "Midimap_wrong.xml", "<drumkit/>");
    file(kKitDir / "Midimap_full.xml", kMidiMapXml);
    file(kKitDir / "Midimap_second.xml", R"(<midimap><map note="38" instr="Kdrum"/></midimap>)");

    const auto result = reader.read(kDrumkit, false);

    ASSERT_TRUE(result.kit);
    EXPECT_EQ(result.kit->midiMap, (std::map<int, std::string>{{36, "Kdrum"}, {42, "Hihat"}}));
}

TEST_F(DrumGizmoReaderTest, FDG01_KitWithoutChannelsOrInstrumentsIsEmpty)
{
    file(kDrumkit, "<drumkit name=\"Empty\"/>");

    const auto result = reader.read(kDrumkit, true);

    ASSERT_TRUE(result.kit);
    EXPECT_TRUE(result.kit->channels.empty());
    EXPECT_TRUE(result.kit->instruments.empty());
    EXPECT_TRUE(result.kit->midiMap.empty());
}

TEST_F(DrumGizmoReaderTest, FDG01_InstrumentWithoutSamplesHasNone)
{
    file(kKitDir / "Hihat/Hihat.xml", "<instrument name=\"Hihat\"/>");

    const auto result = reader.read(kDrumkit, true);

    ASSERT_TRUE(result.kit);
    EXPECT_TRUE(result.kit->instruments[1].samples.empty());
}

TEST_F(DrumGizmoReaderTest, Q09_MissingDrumkitFileIsReported)
{
    const auto result = reader.read(kKitDir / "gone.xml", true);

    EXPECT_FALSE(result.kit);
    EXPECT_EQ(result.error, DrumGizmoError::fileMissing);
    EXPECT_EQ(result.errorFile, kKitDir / "gone.xml");
}

TEST_F(DrumGizmoReaderTest, Q09_BrokenOrForeignDrumkitFileIsNotADrumkit)
{
    for (const auto* text : {"no xml at all", "<instrument name=\"x\"/>"})
    {
        file(kDrumkit, text);

        const auto result = reader.read(kDrumkit, true);

        EXPECT_FALSE(result.kit);
        EXPECT_EQ(result.error, DrumGizmoError::notADrumkit);
        EXPECT_EQ(result.errorFile, kDrumkit);
    }
}

TEST_F(DrumGizmoReaderTest, Q09_MissingInstrumentFileIsReported)
{
    ON_CALL(fileSystem, readText(kKitDir / "Hihat/Hihat.xml")).WillByDefault(Return(std::nullopt));
    ON_CALL(fileSystem, exists(kKitDir / "Hihat/Hihat.xml")).WillByDefault(Return(false));

    const auto result = reader.read(kDrumkit, true);

    EXPECT_FALSE(result.kit);
    EXPECT_EQ(result.error, DrumGizmoError::instrumentMissing);
    EXPECT_EQ(result.errorFile, kKitDir / "Hihat/Hihat.xml");
}

TEST_F(DrumGizmoReaderTest, Q09_BrokenOrForeignInstrumentFileIsReported)
{
    for (const auto* text : {"no xml", "<drumkit/>"})
    {
        file(kKitDir / "Hihat/Hihat.xml", text);

        const auto result = reader.read(kDrumkit, true);

        EXPECT_FALSE(result.kit);
        EXPECT_EQ(result.error, DrumGizmoError::instrumentBroken);
        EXPECT_EQ(result.errorFile, kKitDir / "Hihat/Hihat.xml");
    }
}

TEST_F(DrumGizmoReaderTest, FDG01_FindsInstrumentsByName)
{
    const auto result = reader.read(kDrumkit, false);

    ASSERT_TRUE(result.kit);
    EXPECT_EQ(result.kit->findInstrument("Hihat"), &result.kit->instruments[1]);
    EXPECT_EQ(result.kit->findInstrument("Cowbell"), nullptr);
}

} // namespace
} // namespace drumprog::io
