#include "io/MockProjectRepository.h"
#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "ui/DocumentController.h"
#include "ui/FakeDocumentView.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
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
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Return;

class DocumentControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(repository, save(_, _)).WillByDefault(Return(ProjectFileError::none));
    }

    LoadResult loadableProject(const std::string& name, double bpm)
    {
        Project loaded{factory.createDefault(), nullptr};
        loaded.setName(name);
        loaded.setBpm(bpm);
        return {loaded.tree(), ProjectFileError::none, {}};
    }

    void makeChange() { project.setBpm(project.bpm() + 1.0); }

    void saveAs(const fs::path& file)
    {
        view.fileToSave = file;
        controller.saveAs();
    }

    fs::path rockFile = fs::current_path().root_path() / "projects" / "Rock-Demo.dpp";

    model::FakeIdGenerator idGenerator;
    model::ProjectFactory factory{idGenerator};
    juce::UndoManager undoManager;
    juce::ValueTree tree = factory.createDefault();
    Project project{tree, &undoManager};
    NiceMock<io::MockProjectRepository> repository;
    FakeDocumentView view;
    DocumentController controller{tree, undoManager, repository, factory, view};
};

// ----- Window title and unsaved changes -----------------------------------------------------------

TEST_F(DocumentControllerTest, FPJ02_TitleShowsTheProjectName)
{
    EXPECT_EQ(view.windowTitle, "Neues Projekt – Drum Programmer");
    EXPECT_EQ(controller.windowTitle(), view.windowTitle);
    EXPECT_FALSE(controller.hasUnsavedChanges());
}

TEST_F(DocumentControllerTest, FPJ04_TitleMarksUnsavedChanges)
{
    makeChange();

    EXPECT_TRUE(controller.hasUnsavedChanges());
    EXPECT_EQ(view.windowTitle, "*Neues Projekt – Drum Programmer");
}

TEST_F(DocumentControllerTest, FPJ02_TitleShowsTheFileNameOnceSaved)
{
    saveAs(rockFile);

    EXPECT_EQ(view.windowTitle, "Rock-Demo – Drum Programmer");
}

// ----- Save and save as ---------------------------------------------------------------------------

TEST_F(DocumentControllerTest, FPJ02_SaveAsWritesTheChosenFile)
{
    makeChange();
    EXPECT_CALL(repository, save(tree, rockFile)).WillOnce(Return(ProjectFileError::none));

    saveAs(rockFile);

    EXPECT_EQ(controller.currentFile(), rockFile);
    EXPECT_FALSE(controller.hasUnsavedChanges());
}

TEST_F(DocumentControllerTest, FPJ02_SaveAsSuggestsTheProjectName)
{
    saveAs(rockFile);
    saveAs(rockFile);

    EXPECT_THAT(view.suggestedSaveNames, ::testing::ElementsAre("Neues Projekt.dpp", "Rock-Demo.dpp"));
}

TEST_F(DocumentControllerTest, FPJ02_SaveAsAddsTheFileExtension)
{
    const auto withoutExtension = rockFile.parent_path() / "Rock-Demo";
    EXPECT_CALL(repository, save(_, rockFile)).WillOnce(Return(ProjectFileError::none));

    saveAs(withoutExtension);

    EXPECT_EQ(controller.currentFile(), rockFile);
}

TEST_F(DocumentControllerTest, FPJ02_CancelledSaveAsSavesNothing)
{
    makeChange();
    EXPECT_CALL(repository, save(_, _)).Times(0);
    bool saved = true;

    controller.saveAs([&](bool result) { saved = result; });

    EXPECT_FALSE(saved);
    EXPECT_TRUE(controller.hasUnsavedChanges());
}

TEST_F(DocumentControllerTest, FPJ02_SaveWritesTheCurrentFileWithoutAsking)
{
    saveAs(rockFile);
    makeChange();
    view.fileToSave = std::nullopt;
    EXPECT_CALL(repository, save(tree, rockFile)).WillOnce(Return(ProjectFileError::none));
    bool saved = false;

    controller.save([&](bool result) { saved = result; });

    EXPECT_TRUE(saved);
    EXPECT_FALSE(controller.hasUnsavedChanges());
    EXPECT_EQ(view.suggestedSaveNames.size(), 1U);
}

TEST_F(DocumentControllerTest, FPJ02_SaveOfANewProjectAsksForAFile)
{
    view.fileToSave = rockFile;
    EXPECT_CALL(repository, save(tree, rockFile)).WillOnce(Return(ProjectFileError::none));

    controller.save();

    EXPECT_EQ(controller.currentFile(), rockFile);
}

TEST_F(DocumentControllerTest, FPJ02_FailedSaveKeepsChangesAndTellsTheUser)
{
    makeChange();
    EXPECT_CALL(repository, save(_, _)).WillOnce(Return(ProjectFileError::writeFailed));
    bool saved = true;
    view.fileToSave = rockFile;

    controller.saveAs([&](bool result) { saved = result; });

    EXPECT_FALSE(saved);
    EXPECT_TRUE(controller.hasUnsavedChanges());
    EXPECT_FALSE(controller.currentFile().has_value());
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Speichern fehlgeschlagen");
    EXPECT_THAT(view.messages[0].text, HasSubstr("Rock-Demo.dpp"));
}

// ----- New project --------------------------------------------------------------------------------

TEST_F(DocumentControllerTest, FPJ01_NewProjectReplacesASavedProjectWithoutAsking)
{
    project.setBpm(90.0);
    saveAs(rockFile);

    controller.newProject();

    EXPECT_TRUE(view.askedToSave.empty());
    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);
    EXPECT_FALSE(controller.currentFile().has_value());
    EXPECT_FALSE(controller.hasUnsavedChanges());
    EXPECT_EQ(view.windowTitle, "Neues Projekt – Drum Programmer");
}

TEST_F(DocumentControllerTest, FPJ01_NewProjectGetsANewPatternId)
{
    controller.newProject();

    EXPECT_EQ(project.pattern(0).id(), "id-2");
}

TEST_F(DocumentControllerTest, FPJ04_NewProjectAsksForUnsavedChangesAndCancelKeepsEverything)
{
    project.setBpm(90.0);
    view.saveChangesAnswer = SaveChangesChoice::cancel;

    controller.newProject();

    EXPECT_THAT(view.askedToSave, ::testing::ElementsAre("Neues Projekt"));
    EXPECT_DOUBLE_EQ(project.bpm(), 90.0);
    EXPECT_TRUE(controller.hasUnsavedChanges());
}

TEST_F(DocumentControllerTest, FPJ04_DiscardingChangesCreatesTheNewProject)
{
    project.setBpm(90.0);
    view.saveChangesAnswer = SaveChangesChoice::discard;
    EXPECT_CALL(repository, save(_, _)).Times(0);

    controller.newProject();

    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);
}

TEST_F(DocumentControllerTest, FPJ04_SavingChangesFirstSavesThenCreatesTheNewProject)
{
    saveAs(rockFile);
    project.setBpm(90.0);
    view.saveChangesAnswer = SaveChangesChoice::save;
    EXPECT_CALL(repository, save(_, rockFile)).WillOnce(Return(ProjectFileError::none));

    controller.newProject();

    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);
}

TEST_F(DocumentControllerTest, FPJ04_NothingHappensIfSavingTheChangesFails)
{
    project.setBpm(90.0);
    view.saveChangesAnswer = SaveChangesChoice::save;
    view.fileToSave = std::nullopt; // user cancels the file dialog

    controller.newProject();

    EXPECT_DOUBLE_EQ(project.bpm(), 90.0);
}

TEST_F(DocumentControllerTest, FPJ05_NewProjectStartsWithEmptyUndoHistory)
{
    undoManager.beginNewTransaction();
    makeChange();
    view.saveChangesAnswer = SaveChangesChoice::discard;

    controller.newProject();

    EXPECT_FALSE(controller.canUndo());
}

// ----- Open ---------------------------------------------------------------------------------------

TEST_F(DocumentControllerTest, FPJ02_OpenLoadsTheChosenFileIntoTheLiveTree)
{
    view.fileToOpen = rockFile;
    EXPECT_CALL(repository, load(rockFile)).WillOnce(Return(loadableProject("Rock-Demo", 97.0)));

    controller.open();

    EXPECT_DOUBLE_EQ(project.bpm(), 97.0);
    EXPECT_EQ(project.name(), "Rock-Demo");
    EXPECT_EQ(controller.currentFile(), rockFile);
    EXPECT_FALSE(controller.hasUnsavedChanges());
    EXPECT_FALSE(controller.canUndo());
    EXPECT_EQ(view.windowTitle, "Rock-Demo – Drum Programmer");
    EXPECT_TRUE(view.messages.empty());
}

TEST_F(DocumentControllerTest, FPJ02_CancelledOpenLoadsNothing)
{
    view.fileToOpen = std::nullopt;
    EXPECT_CALL(repository, load(_)).Times(0);

    controller.open();
}

TEST_F(DocumentControllerTest, FPJ04_OpenAsksForUnsavedChangesFirst)
{
    makeChange();
    view.saveChangesAnswer = SaveChangesChoice::cancel;
    view.fileToOpen = rockFile;
    EXPECT_CALL(repository, load(_)).Times(0);

    controller.open();

    EXPECT_EQ(view.askedToSave.size(), 1U);
}

TEST_F(DocumentControllerTest, FPJ02_FailedOpenKeepsTheCurrentProject)
{
    project.setBpm(90.0);
    saveAs(rockFile);
    view.fileToOpen = rockFile.parent_path() / "broken.dpp";
    EXPECT_CALL(repository, load(_)).WillOnce(Return(LoadResult{{}, ProjectFileError::unreadable, {}}));

    controller.open();

    EXPECT_DOUBLE_EQ(project.bpm(), 90.0);
    EXPECT_EQ(controller.currentFile(), rockFile);
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Öffnen fehlgeschlagen");
    EXPECT_THAT(view.messages[0].text, HasSubstr("broken.dpp"));
    EXPECT_THAT(view.messages[0].text, HasSubstr("konnte nicht gelesen werden"));
}

TEST_F(DocumentControllerTest, FPJ02_ExplainsFileThatIsNoProject)
{
    view.fileToOpen = rockFile;
    EXPECT_CALL(repository, load(_)).WillOnce(Return(LoadResult{{}, ProjectFileError::notAProject, {}}));

    controller.open();

    EXPECT_THAT(view.messages.at(0).text, HasSubstr("keine Drum-Programmer-Projektdatei"));
}

TEST_F(DocumentControllerTest, FPJ02_ExplainsFileFromNewerVersion)
{
    view.fileToOpen = rockFile;
    EXPECT_CALL(repository, load(_))
        .WillOnce(Return(LoadResult{{}, ProjectFileError::newerFormatVersion, {}}));

    controller.open();

    EXPECT_THAT(view.messages.at(0).text, HasSubstr("neueren Version"));
}

TEST_F(DocumentControllerTest, FPJ03_ListsMissingSamplesAfterOpening)
{
    view.fileToOpen = rockFile;
    auto result = loadableProject("Rock-Demo", 120.0);
    result.missingSamples = {{"Acoustic Snare", "/samples/snare.wav"}, {"Crash Cymbal 1", "/samples/crash.wav"}};
    EXPECT_CALL(repository, load(_)).WillOnce(Return(result));

    controller.open();

    EXPECT_EQ(controller.currentFile(), rockFile);
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Fehlende Samples");
    EXPECT_THAT(view.messages[0].text, HasSubstr("kein Sample"));
    EXPECT_THAT(view.messages[0].text, HasSubstr("Acoustic Snare: /samples/snare.wav\n"));
    EXPECT_THAT(view.messages[0].text, HasSubstr("Crash Cymbal 1: /samples/crash.wav\n"));
}

// ----- Close --------------------------------------------------------------------------------------

TEST_F(DocumentControllerTest, FPJ04_ClosesWithoutAskingWhenEverythingIsSaved)
{
    bool closed = false;

    controller.requestClose([&] { closed = true; });

    EXPECT_TRUE(closed);
    EXPECT_TRUE(view.askedToSave.empty());
}

TEST_F(DocumentControllerTest, FPJ04_CancelKeepsTheWindowOpen)
{
    makeChange();
    view.saveChangesAnswer = SaveChangesChoice::cancel;
    bool closed = false;

    controller.requestClose([&] { closed = true; });

    EXPECT_FALSE(closed);
}

TEST_F(DocumentControllerTest, FPJ04_DiscardCloses)
{
    makeChange();
    view.saveChangesAnswer = SaveChangesChoice::discard;
    bool closed = false;

    controller.requestClose([&] { closed = true; });

    EXPECT_TRUE(closed);
}

// ----- Undo and redo ------------------------------------------------------------------------------

TEST_F(DocumentControllerTest, FPJ05_UndoAndRedoTheLastEdit)
{
    EXPECT_FALSE(controller.canUndo());
    undoManager.beginNewTransaction();
    project.setBpm(90.0);
    ASSERT_TRUE(controller.canUndo());

    controller.undo();
    EXPECT_DOUBLE_EQ(project.bpm(), 120.0);
    ASSERT_TRUE(controller.canRedo());

    controller.redo();
    EXPECT_DOUBLE_EQ(project.bpm(), 90.0);
    EXPECT_FALSE(controller.canRedo());
}

TEST_F(DocumentControllerTest, FPJ05_UndoCountsAsUnsavedChange)
{
    undoManager.beginNewTransaction();
    project.setBpm(90.0);
    saveAs(rockFile);

    controller.undo();

    EXPECT_TRUE(controller.hasUnsavedChanges());
}

} // namespace
} // namespace drumprog::ui
