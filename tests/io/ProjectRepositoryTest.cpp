#include "io/ProjectRepository.h"
#include "TestComparisons.h"
#include "io/MockFileSystem.h"
#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace drumprog::io
{
namespace
{

namespace fs = std::filesystem;
using model::Project;
using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

class ProjectRepositoryTest : public ::testing::Test
{
protected:
    fs::path projectDir = fs::current_path().root_path() / "projects";
    fs::path projectFile = projectDir / "rock.dpp";
    fs::path tempFile = projectDir / "rock.dpp.tmp";
    fs::path kickFile = projectDir / "samples" / "kick.wav";
    fs::path snareFile = projectDir / "samples" / "snare.wav";

    model::FakeIdGenerator idGenerator;
    juce::ValueTree tree = model::ProjectFactory{idGenerator}.createDefault();
    Project project{tree, nullptr};
    NiceMock<MockFileSystem> fileSystem;
    ProjectRepository repository{fileSystem};

    void givenProjectFileWithSamples()
    {
        project.kit().findSlot(36)->setFilePath(kickFile.string());
        project.kit().findSlot(38)->setFilePath(snareFile.string());
        ON_CALL(fileSystem, readText(projectFile))
            .WillByDefault(Return(ProjectSerializer::toXml(tree, projectDir)));
    }
};

TEST_F(ProjectRepositoryTest, Q10_SavesIntoTemporaryFileAndRenamesIt)
{
    std::string written;
    ::testing::InSequence sequence;
    EXPECT_CALL(fileSystem, writeText(tempFile, _)).WillOnce(DoAll(SaveArg<1>(&written), Return(true)));
    EXPECT_CALL(fileSystem, rename(tempFile, projectFile)).WillOnce(Return(true));

    EXPECT_EQ(repository.save(tree, projectFile), ProjectFileError::none);
    EXPECT_EQ(written, ProjectSerializer::toXml(tree, projectDir));
}

TEST_F(ProjectRepositoryTest, Q10_ReportsFailedWriteAndKeepsTheOldFile)
{
    EXPECT_CALL(fileSystem, writeText(tempFile, _)).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, rename(_, _)).Times(0);

    EXPECT_EQ(repository.save(tree, projectFile), ProjectFileError::writeFailed);
}

TEST_F(ProjectRepositoryTest, Q10_ReportsFailedRename)
{
    EXPECT_CALL(fileSystem, writeText(tempFile, _)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, rename(tempFile, projectFile)).WillOnce(Return(false));

    EXPECT_EQ(repository.save(tree, projectFile), ProjectFileError::writeFailed);
}

TEST_F(ProjectRepositoryTest, FPJ02_LoadsSavedProject)
{
    project.setName("Rock-Demo");
    givenProjectFileWithSamples();
    ON_CALL(fileSystem, exists(_)).WillByDefault(Return(true));

    const auto result = repository.load(projectFile);

    ASSERT_EQ(result.error, ProjectFileError::none);
    EXPECT_EQ(Project(result.project, nullptr).name(), "Rock-Demo");
    EXPECT_TRUE(result.missingSamples.empty());
}

TEST_F(ProjectRepositoryTest, FPJ02_ReportsUnreadableFile)
{
    EXPECT_CALL(fileSystem, readText(projectFile)).WillOnce(Return(std::nullopt));

    EXPECT_EQ(repository.load(projectFile).error, ProjectFileError::unreadable);
}

TEST_F(ProjectRepositoryTest, FPJ02_ReportsFileThatIsNoProject)
{
    EXPECT_CALL(fileSystem, readText(projectFile)).WillOnce(Return(std::string{"<DEVICESETUP/>"}));

    EXPECT_EQ(repository.load(projectFile).error, ProjectFileError::notAProject);
}

TEST_F(ProjectRepositoryTest, FPJ03_ReportsAndMarksMissingSamplesInsteadOfFailing)
{
    givenProjectFileWithSamples();
    EXPECT_CALL(fileSystem, exists(kickFile)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, exists(snareFile)).WillOnce(Return(false));

    const auto result = repository.load(projectFile);

    ASSERT_EQ(result.error, ProjectFileError::none);
    ASSERT_EQ(result.missingSamples.size(), 1U);
    EXPECT_EQ(result.missingSamples[0], (MissingSample{"Acoustic Snare", snareFile.string()}));
    const Project loaded{result.project, nullptr};
    EXPECT_TRUE(loaded.kit().findSlot(36)->hasSample());
    EXPECT_FALSE(loaded.kit().findSlot(38)->hasSample());
    EXPECT_TRUE(loaded.kit().findSlot(38)->isSampleMissing());
}

TEST_F(ProjectRepositoryTest, FPJ03_SlotsWithoutFileAreNotChecked)
{
    ON_CALL(fileSystem, readText(projectFile))
        .WillByDefault(Return(ProjectSerializer::toXml(tree, projectDir)));
    EXPECT_CALL(fileSystem, exists(_)).Times(0);

    EXPECT_TRUE(repository.load(projectFile).missingSamples.empty());
}

} // namespace
} // namespace drumprog::io
