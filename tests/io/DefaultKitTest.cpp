#include "io/DefaultKit.h"
#include "io/MockFileSystem.h"
#include "model/Project.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace drumprog::io
{
namespace
{

namespace fs = std::filesystem;
using ::testing::_;
using ::testing::EndsWith;
using ::testing::NiceMock;
using ::testing::Return;

const fs::path kDirectory = fs::current_path().root_path() / "config" / "DefaultKit";

class DefaultKitTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(fileSystem, createDirectories(kDirectory)).WillByDefault(Return(true));
        ON_CALL(fileSystem, writeText(_, _))
            .WillByDefault(
                [this](const fs::path& file, const std::string& bytes)
                {
                    written[file] = bytes;
                    return true;
                });
        ON_CALL(fileSystem, rename(_, _))
            .WillByDefault(
                [this](const fs::path& from, const fs::path& to)
                {
                    renamed.insert({from, to});
                    return true;
                });
    }

    /// Every built-in file holds its own name as bytes.
    static std::optional<std::string> resource(std::string_view fileName) { return std::string{fileName}; }

    [[nodiscard]] static std::string sampleOf(const model::Kit& kit, int gmNote)
    {
        return kit.findSlot(gmNote)->filePath();
    }

    NiceMock<MockFileSystem> fileSystem;
    std::map<fs::path, std::string> written;
    std::set<std::pair<fs::path, fs::path>> renamed;
    DefaultKit defaultKit{fileSystem, kDirectory, &resource};
};

TEST_F(DefaultKitTest, FSE04_EveryGmInstrumentButTheVibraslapGetsTheLoudestSample)
{
    const auto tree = defaultKit.createKit();
    const model::Kit kit{tree, nullptr};

    EXPECT_EQ(kit.numSlots(), 25);
    for (int index = 0; index < kit.numSlots(); ++index)
    {
        const auto slot = kit.slot(index);
        EXPECT_EQ(slot.hasSample(), slot.gmNote() != 58) << slot.gmNote();
        if (slot.hasSample())
        {
            EXPECT_THAT(slot.filePath(), EndsWith("-5.wav")) << slot.gmNote();
        }
    }
    EXPECT_EQ(fs::path{sampleOf(kit, 38)}, kDirectory / "38-Pearl-14-Snare-5.wav");
    EXPECT_EQ(fs::path{sampleOf(kit, 35)}, fs::path{sampleOf(kit, 36)});
    EXPECT_EQ(fs::path{sampleOf(kit, 52)}, kDirectory / "62-Wuhan-18-ChinaCrash-5.wav");
    EXPECT_EQ(DefaultKit::samples().size(), 24U);
}

TEST_F(DefaultKitTest, FSE04_KeepsTheGmNotesNamesAndHiHatChokeGroup)
{
    const auto tree = defaultKit.createKit();
    const model::Kit kit{tree, nullptr};

    const auto hiHat = *kit.findSlot(42);
    EXPECT_EQ(hiHat.midiNote(), 42);
    EXPECT_EQ(hiHat.name(), "Closed Hi-Hat");
    EXPECT_EQ(hiHat.chokeGroup(), 1);
    EXPECT_DOUBLE_EQ(hiHat.gain(), 1.0);
}

TEST_F(DefaultKitTest, FSE04_FirstStartCopiesEveryFileOnceThroughATemporaryFile)
{
    ON_CALL(fileSystem, exists(_)).WillByDefault(Return(false));

    EXPECT_TRUE(defaultKit.install());

    const auto snare = kDirectory / "38-Pearl-14-Snare-5.wav";
    auto partial = snare;
    partial += ".part";
    EXPECT_EQ(written.at(partial), "38-Pearl-14-Snare-5.wav");
    EXPECT_EQ(renamed.count({partial, snare}), 1U);
    EXPECT_EQ(renamed.count({kDirectory / "LIESMICH.txt.part", kDirectory / "LIESMICH.txt"}), 1U);
    // 24 slots share 21 samples, plus the license text.
    EXPECT_EQ(written.size(), 22U);
}

TEST_F(DefaultKitTest, FSE04_LaterStartsKeepTheCopiedFiles)
{
    ON_CALL(fileSystem, exists(_)).WillByDefault(Return(true));
    EXPECT_CALL(fileSystem, writeText(_, _)).Times(0);

    EXPECT_TRUE(defaultKit.install());
}

TEST_F(DefaultKitTest, Q10_ReportsAFileThatCouldNotBeCopiedAndCopiesTheOthers)
{
    ON_CALL(fileSystem, exists(_)).WillByDefault(Return(false));
    ON_CALL(fileSystem, rename(kDirectory / "LIESMICH.txt.part", _)).WillByDefault(Return(false));

    EXPECT_FALSE(defaultKit.install());

    EXPECT_EQ(written.size(), 22U);
}

TEST_F(DefaultKitTest, Q10_FailsWithoutTheBuiltInFileOrTheDirectory)
{
    ON_CALL(fileSystem, exists(_)).WillByDefault(Return(false));
    DefaultKit withoutResources{fileSystem, kDirectory, [](std::string_view) { return std::nullopt; }};
    EXPECT_FALSE(withoutResources.install());
    EXPECT_TRUE(written.empty());

    ON_CALL(fileSystem, writeText(_, _)).WillByDefault(Return(false));
    EXPECT_FALSE(defaultKit.install());

    ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(false));
    EXPECT_FALSE(defaultKit.install());
}

} // namespace
} // namespace drumprog::io
