#include "io/GlobalKit.h"
#include "TestComparisons.h"
#include "io/MockFileSystem.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <string>

namespace drumprog::io
{
namespace
{

namespace fs = std::filesystem;
using model::Kit;
using ::testing::_;
using ::testing::DoAll;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Not;
using ::testing::Return;
using ::testing::SaveArg;

const fs::path kKitFile{"config/DrumProgrammer/kit.xml"};
const fs::path kKickFile = fs::current_path().root_path() / "samples" / "kick.wav";
const fs::path kSnareFile = fs::current_path().root_path() / "samples" / "snare.wav";

class GlobalKitTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(true));
        ON_CALL(fileSystem, writeText(kKitFile, _)).WillByDefault(DoAll(SaveArg<1>(&written), Return(true)));
    }

    void givenSavedKit(const std::optional<std::string>& xml)
    {
        ON_CALL(fileSystem, readText(kKitFile)).WillByDefault(Return(xml));
    }

    static std::string kitWithSamples()
    {
        auto kit = model::ProjectFactory::createDefaultKit();
        Kit{kit, nullptr}.findSlot(36)->setFilePath(kKickFile.string());
        Kit{kit, nullptr}.findSlot(38)->setFilePath(kSnareFile.string());
        return kit.toXmlString().toStdString();
    }

    NiceMock<MockFileSystem> fileSystem;
    SettingsStore store{fileSystem, kKitFile};
    std::string written;
};

TEST_F(GlobalKitTest, FSE04_StartsAsTheGmDefaultKitWithoutSavedKit)
{
    givenSavedKit(std::nullopt);

    const GlobalKit kit{store, fileSystem};

    EXPECT_TRUE(kit.tree().isEquivalentTo(model::ProjectFactory::createDefaultKit()));
    EXPECT_TRUE(kit.missingSamples().empty());
    EXPECT_FALSE(kit.hasUnsavedChanges());
}

TEST_F(GlobalKitTest, FSE04_StartsAsTheGmDefaultKitIfTheSavedKitIsNoKit)
{
    givenSavedKit(std::string{"<PROJECT/>"});

    const GlobalKit kit{store, fileSystem};

    EXPECT_EQ(Kit(kit.tree(), nullptr).numSlots(), 25);
}

TEST_F(GlobalKitTest, FPJ02_RestoresTheSavedKit)
{
    givenSavedKit(kitWithSamples());
    ON_CALL(fileSystem, exists(_)).WillByDefault(Return(true));

    const GlobalKit kit{store, fileSystem};

    EXPECT_EQ(fs::path{Kit(kit.tree(), nullptr).findSlot(36)->filePath()}, kKickFile);
    EXPECT_TRUE(kit.missingSamples().empty());
    EXPECT_FALSE(kit.hasUnsavedChanges());
}

TEST_F(GlobalKitTest, FPJ03_MarksAndReportsMissingSamples)
{
    givenSavedKit(kitWithSamples());
    ON_CALL(fileSystem, exists(kKickFile)).WillByDefault(Return(true));
    ON_CALL(fileSystem, exists(kSnareFile)).WillByDefault(Return(false));

    const GlobalKit kit{store, fileSystem};

    ASSERT_EQ(kit.missingSamples().size(), 1U);
    EXPECT_EQ(kit.missingSamples()[0], (MissingSample{"Acoustic Snare", kSnareFile.string()}));
    EXPECT_TRUE(Kit(kit.tree(), nullptr).findSlot(38)->isSampleMissing());
    EXPECT_FALSE(kit.hasUnsavedChanges());
}

TEST_F(GlobalKitTest, FPJ02_SavesOnlyAfterAChange)
{
    GlobalKit kit{store, fileSystem};
    EXPECT_CALL(fileSystem, writeText(_, _)).Times(0);
    EXPECT_TRUE(kit.saveIfChanged());
    ::testing::Mock::VerifyAndClearExpectations(&fileSystem);
    ON_CALL(fileSystem, writeText(kKitFile, _)).WillByDefault(DoAll(SaveArg<1>(&written), Return(true)));

    Kit{kit.tree(), nullptr}.findSlot(38)->setGain(0.5);
    EXPECT_TRUE(kit.hasUnsavedChanges());
    EXPECT_TRUE(kit.saveIfChanged());

    EXPECT_FALSE(kit.hasUnsavedChanges());
    const auto saved = juce::ValueTree::fromXml(juce::String{written});
    EXPECT_DOUBLE_EQ(Kit(saved, nullptr).findSlot(38)->gain(), 0.5);
}

TEST_F(GlobalKitTest, FPJ03_DoesNotSaveTheMissingSampleMarker)
{
    givenSavedKit(kitWithSamples());
    ON_CALL(fileSystem, exists(_)).WillByDefault(Return(false));
    GlobalKit kit{store, fileSystem};

    Kit{kit.tree(), nullptr}.findSlot(36)->setGain(0.5);
    ASSERT_TRUE(kit.saveIfChanged());

    EXPECT_THAT(written, Not(HasSubstr("sampleMissing")));
    EXPECT_THAT(written, HasSubstr("snare.wav"));
}

TEST_F(GlobalKitTest, Q10_AFailedSaveKeepsTheChangesUnsaved)
{
    GlobalKit kit{store, fileSystem};
    Kit{kit.tree(), nullptr}.findSlot(38)->setGain(0.5);
    ON_CALL(fileSystem, writeText(_, _)).WillByDefault(Return(false));

    EXPECT_FALSE(kit.saveIfChanged());

    EXPECT_TRUE(kit.hasUnsavedChanges());
}

} // namespace
} // namespace drumprog::io
