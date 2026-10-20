#include "ui/Autosave.h"
#include "io/MockFileSystem.h"
#include "io/MockProjectRepository.h"
#include "io/Utf8Path.h"
#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "ui/DocumentController.h"
#include "ui/FakeDocumentView.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <string>

namespace drumprog::ui
{
namespace
{

namespace fs = std::filesystem;
using io::LoadResult;
using io::ProjectFileError;
using model::Project;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class AutosaveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(repository, save(_, _)).WillByDefault(Return(ProjectFileError::none));
        ON_CALL(fileSystem, writeText(_, _)).WillByDefault(Return(true));
        ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(true));
        ON_CALL(fileSystem, remove(_)).WillByDefault(Return(true));
    }

    void makeChange() { project.setBpm(project.bpm() + 1.0); }

    /// The first tick starts the interval; the second one after it writes.
    void tickPastTheInterval()
    {
        autosave.tick(now);
        now += Autosave::kIntervalSeconds;
        autosave.tick(now);
    }

    LoadResult backupProject(double bpm)
    {
        Project loaded{factory.createDefault(), nullptr};
        loaded.setBpm(bpm);
        return {loaded.tree(), ProjectFileError::none, {}};
    }

    fs::path directory = fs::current_path().root_path() / "settings";
    Autosave::Files files{.backup = directory / "autosave.dpp", .source = directory / "autosave-source.txt"};
    fs::path rockFile = fs::current_path().root_path() / "projects" / "Rock-Demo.dpp";
    double now = 1000.0;

    model::FakeIdGenerator idGenerator;
    model::ProjectFactory factory{idGenerator};
    juce::UndoManager undoManager;
    juce::ValueTree tree = factory.createDefault();
    Project project{tree, &undoManager};
    NiceMock<io::MockProjectRepository> repository;
    NiceMock<io::MockFileSystem> fileSystem;
    FakeDocumentView view;
    DocumentController document{tree, undoManager, repository, factory, view};
    Autosave autosave{tree, document, repository, fileSystem, files, view};
};

// ----- Writing the backup -------------------------------------------------------------------------

TEST_F(AutosaveTest, Q10_WritesUnsavedChangesAfterFiveMinutes)
{
    EXPECT_DOUBLE_EQ(Autosave::kIntervalSeconds, 300.0);
    makeChange();
    EXPECT_CALL(repository, save(_, _)).Times(0);
    autosave.tick(now);
    autosave.tick(now + Autosave::kIntervalSeconds - 1.0);
    ::testing::Mock::VerifyAndClearExpectations(&repository);

    EXPECT_CALL(fileSystem, createDirectories(directory)).WillOnce(Return(true));
    EXPECT_CALL(repository, save(tree, files.backup)).WillOnce(Return(ProjectFileError::none));
    EXPECT_CALL(fileSystem, writeText(files.source, "")).WillOnce(Return(true));
    autosave.tick(now + Autosave::kIntervalSeconds);
}

TEST_F(AutosaveTest, Q10_RemembersTheProjectFileOfTheBackup)
{
    view.fileToSave = rockFile;
    document.saveAs();
    makeChange();

    EXPECT_CALL(fileSystem, writeText(files.source, io::utf8FromPath(rockFile))).WillOnce(Return(true));
    tickPastTheInterval();
}

TEST_F(AutosaveTest, Q10_WritesNothingWithoutUnsavedChanges)
{
    EXPECT_CALL(repository, save(_, _)).Times(0);

    tickPastTheInterval();
}

TEST_F(AutosaveTest, Q10_WritesNothingIfNothingChangedSinceTheLastBackup)
{
    makeChange();
    tickPastTheInterval();

    EXPECT_CALL(repository, save(_, _)).Times(0);
    now += Autosave::kIntervalSeconds;
    autosave.tick(now);
}

TEST_F(AutosaveTest, Q10_WritesAgainAfterTheNextChange)
{
    makeChange();
    tickPastTheInterval();
    makeChange();

    EXPECT_CALL(repository, save(tree, files.backup)).WillOnce(Return(ProjectFileError::none));
    now += Autosave::kIntervalSeconds;
    autosave.tick(now);
}

TEST_F(AutosaveTest, Q10_TriesAgainAfterAFailedBackup)
{
    makeChange();
    EXPECT_CALL(repository, save(_, _)).WillOnce(Return(ProjectFileError::writeFailed));
    tickPastTheInterval();
    ::testing::Mock::VerifyAndClearExpectations(&repository);

    EXPECT_CALL(repository, save(tree, files.backup)).WillOnce(Return(ProjectFileError::none));
    now += Autosave::kIntervalSeconds;
    autosave.tick(now);
    EXPECT_TRUE(view.messages.empty());
}

TEST_F(AutosaveTest, Q10_TriesAgainIfTheProjectFileWasNotRemembered)
{
    makeChange();
    EXPECT_CALL(fileSystem, writeText(files.source, _)).WillOnce(Return(false)).WillOnce(Return(true));
    tickPastTheInterval();

    EXPECT_CALL(repository, save(tree, files.backup)).WillOnce(Return(ProjectFileError::none));
    now += Autosave::kIntervalSeconds;
    autosave.tick(now);
}

// ----- Removing the backup ------------------------------------------------------------------------

TEST_F(AutosaveTest, Q10_SavingRemovesTheBackup)
{
    makeChange();
    tickPastTheInterval();

    EXPECT_CALL(fileSystem, remove(files.backup)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, remove(files.source)).WillOnce(Return(true));
    view.fileToSave = rockFile;
    document.saveAs();
}

TEST_F(AutosaveTest, Q10_ABackupAfterSavingNeedsANewChange)
{
    makeChange();
    tickPastTheInterval();
    view.fileToSave = rockFile;
    document.saveAs();

    EXPECT_CALL(repository, save(_, files.backup)).Times(0);
    now += Autosave::kIntervalSeconds;
    autosave.tick(now);
}

TEST_F(AutosaveTest, Q10_DiscardRemovesTheBackup)
{
    EXPECT_CALL(fileSystem, remove(files.backup)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, remove(files.source)).WillOnce(Return(true));

    autosave.discard();
}

// ----- Recovery at program start ------------------------------------------------------------------

TEST_F(AutosaveTest, Q10_AsksNothingWithoutABackup)
{
    EXPECT_CALL(fileSystem, exists(files.backup)).WillOnce(Return(false));

    autosave.offerRecovery();

    EXPECT_EQ(view.askedToRestore, 0);
}

TEST_F(AutosaveTest, Q10_RestoresTheBackupOfASavedProject)
{
    EXPECT_CALL(fileSystem, exists(files.backup)).WillOnce(Return(true));
    EXPECT_CALL(repository, load(files.backup)).WillOnce(Return(backupProject(95.0)));
    EXPECT_CALL(fileSystem, readText(files.source))
        .WillOnce(Return(std::optional<std::string>{io::utf8FromPath(rockFile)}));
    EXPECT_CALL(fileSystem, remove(_)).Times(0);
    view.restoreAnswer = true;

    autosave.offerRecovery();

    EXPECT_EQ(view.askedToRestore, 1);
    EXPECT_DOUBLE_EQ(project.bpm(), 95.0);
    EXPECT_EQ(document.currentFile(), rockFile);
    EXPECT_TRUE(document.hasUnsavedChanges());
}

TEST_F(AutosaveTest, Q10_RestoresTheBackupOfANewProject)
{
    ON_CALL(fileSystem, exists(files.backup)).WillByDefault(Return(true));
    ON_CALL(repository, load(files.backup)).WillByDefault(Return(backupProject(95.0)));
    ON_CALL(fileSystem, readText(files.source)).WillByDefault(Return(std::optional<std::string>{""}));
    view.restoreAnswer = true;

    autosave.offerRecovery();

    EXPECT_FALSE(document.currentFile().has_value());
    EXPECT_TRUE(document.hasUnsavedChanges());
}

TEST_F(AutosaveTest, Q10_RestoresWithoutTheRememberedFile)
{
    ON_CALL(fileSystem, exists(files.backup)).WillByDefault(Return(true));
    ON_CALL(repository, load(files.backup)).WillByDefault(Return(backupProject(95.0)));
    ON_CALL(fileSystem, readText(files.source)).WillByDefault(Return(std::nullopt));
    view.restoreAnswer = true;

    autosave.offerRecovery();

    EXPECT_FALSE(document.currentFile().has_value());
    EXPECT_DOUBLE_EQ(project.bpm(), 95.0);
}

TEST_F(AutosaveTest, Q10_TheRestoredBackupIsOnlyWrittenAgainAfterAChange)
{
    ON_CALL(fileSystem, exists(files.backup)).WillByDefault(Return(true));
    ON_CALL(repository, load(files.backup)).WillByDefault(Return(backupProject(95.0)));
    view.restoreAnswer = true;
    autosave.offerRecovery();

    EXPECT_CALL(repository, save(_, _)).Times(0);
    tickPastTheInterval();
}

TEST_F(AutosaveTest, Q10_DecliningRemovesTheBackup)
{
    ON_CALL(fileSystem, exists(files.backup)).WillByDefault(Return(true));
    EXPECT_CALL(repository, load(_)).Times(0);
    EXPECT_CALL(fileSystem, remove(files.backup)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, remove(files.source)).WillOnce(Return(true));
    view.restoreAnswer = false;

    autosave.offerRecovery();

    EXPECT_FALSE(document.hasUnsavedChanges());
}

TEST_F(AutosaveTest, Q10_AnUnreadableBackupIsReportedAndRemoved)
{
    ON_CALL(fileSystem, exists(files.backup)).WillByDefault(Return(true));
    EXPECT_CALL(repository, load(files.backup))
        .WillOnce(Return(LoadResult{{}, ProjectFileError::notAProject, {}}));
    EXPECT_CALL(fileSystem, remove(files.backup)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, remove(files.source)).WillOnce(Return(true));
    view.restoreAnswer = true;

    autosave.offerRecovery();

    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Wiederherstellen fehlgeschlagen");
    EXPECT_FALSE(document.hasUnsavedChanges());
}

} // namespace
} // namespace drumprog::ui
