#pragma once

#include "io/IProjectRepository.h"
#include "model/ProjectFactory.h"
#include "model/TreeChangeListener.h"
#include "ui/IDocumentView.h"

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace drumprog::ui
{

/// Presenter for the menus "Datei" and "Bearbeiten": new, open, save, save as, close with the
/// question for unsaved changes (F-PJ-01 to 04) and undo/redo (F-PJ-05).
///
/// It works on one project tree that lives as long as the application, so listeners such as the
/// SnapshotPublisher stay attached: new and opened projects are copied into it, and their undo
/// history starts empty.
class DocumentController
{
public:
    static constexpr const char* kFileExtension = ".dpp";

    DocumentController(juce::ValueTree project,
                       juce::UndoManager& undoManager,
                       io::IProjectRepository& repository,
                       const model::ProjectFactory& factory,
                       IDocumentView& view);

    void newProject();
    void open();
    /// Opens the file without a file dialog, e.g. one given to the program; asks for unsaved changes first.
    void open(const std::filesystem::path& file);
    void save(std::function<void(bool saved)> onDone = [](bool) {});
    void saveAs(std::function<void(bool saved)> onDone = [](bool) {});
    /// Calls onClose unless the user cancels the question for unsaved changes.
    void requestClose(std::function<void()> onClose);

    /// Reports samples of the global kit that were missing at program start; nothing if none.
    void reportMissingGlobalKitSamples(const std::vector<io::MissingSample>& missing);

    /// Replaces the project with a backup of unsaved changes (Q-10); they stay unsaved. file is the project
    /// file the backup belongs to, none for a project that was never saved.
    void restoreBackup(const io::LoadResult& backup, std::optional<std::filesystem::path> file);
    /// Called whenever the project has no unsaved changes any more: saved, replaced or opened.
    void setOnNoUnsavedChanges(std::function<void()> callback) { onNoUnsavedChanges_ = std::move(callback); }

    void undo();
    void redo();
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    [[nodiscard]] bool hasUnsavedChanges() const { return unsavedChanges_; }
    [[nodiscard]] const std::optional<std::filesystem::path>& currentFile() const { return currentFile_; }
    [[nodiscard]] std::string windowTitle() const;

    /// The first .dpp file among the program's command line arguments, also in quotes; none if there is none.
    [[nodiscard]] static std::optional<std::filesystem::path>
    projectFileIn(const std::vector<std::string>& arguments);

private:
    void ifChangesSaved(std::function<void()> action);
    void openFile(const std::filesystem::path& file);
    bool saveTo(const std::filesystem::path& file);
    void replaceProject(const juce::ValueTree& newProject,
                        std::optional<std::filesystem::path> file,
                        bool unsavedChanges = false);
    void reportMissingSamples(const std::vector<io::MissingSample>& missing);
    void setUnsavedChanges(bool unsavedChanges);
    [[nodiscard]] std::string displayName() const;

    [[nodiscard]] static std::filesystem::path withFileExtension(std::filesystem::path file);
    [[nodiscard]] static std::string loadErrorMessage(io::ProjectFileError error,
                                                      const std::string& fileName);
    [[nodiscard]] static std::string missingSamplesMessage(const std::vector<io::MissingSample>& missing);

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
    io::IProjectRepository& repository_;
    const model::ProjectFactory& factory_;
    IDocumentView& view_;
    std::optional<std::filesystem::path> currentFile_;
    bool unsavedChanges_ = false;
    std::function<void()> onNoUnsavedChanges_ = [] {};
    model::TreeChangeListener listener_;
};

} // namespace drumprog::ui
