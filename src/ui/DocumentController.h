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
    void save(std::function<void(bool saved)> onDone = [](bool) {});
    void saveAs(std::function<void(bool saved)> onDone = [](bool) {});
    /// Calls onClose unless the user cancels the question for unsaved changes.
    void requestClose(std::function<void()> onClose);

    void undo();
    void redo();
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    [[nodiscard]] bool hasUnsavedChanges() const { return unsavedChanges_; }
    [[nodiscard]] const std::optional<std::filesystem::path>& currentFile() const { return currentFile_; }
    [[nodiscard]] std::string windowTitle() const;

private:
    void ifChangesSaved(std::function<void()> action);
    void openFile(const std::filesystem::path& file);
    bool saveTo(const std::filesystem::path& file);
    void replaceProject(const juce::ValueTree& newProject, std::optional<std::filesystem::path> file);
    void setUnsavedChanges(bool unsavedChanges);
    [[nodiscard]] std::string displayName() const;

    [[nodiscard]] static std::filesystem::path withFileExtension(std::filesystem::path file);
    [[nodiscard]] static std::string loadErrorMessage(io::ProjectFileError error);
    [[nodiscard]] static std::string missingSamplesMessage(const std::vector<io::MissingSample>& missing);

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
    io::IProjectRepository& repository_;
    const model::ProjectFactory& factory_;
    IDocumentView& view_;
    std::optional<std::filesystem::path> currentFile_;
    bool unsavedChanges_ = false;
    model::TreeChangeListener listener_;
};

} // namespace drumprog::ui
