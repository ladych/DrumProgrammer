#include "ui/DocumentController.h"

#include "io/Utf8Path.h"
#include "model/Project.h"

#include <utility>

namespace drumprog::ui
{
namespace
{

constexpr const char* kApplicationName = "Drum Programmer";
constexpr const char* kUnsavedMarker = "*";

} // namespace

DocumentController::DocumentController(juce::ValueTree project,
                                       juce::UndoManager& undoManager,
                                       io::IProjectRepository& repository,
                                       const model::ProjectFactory& factory,
                                       IDocumentView& view)
    : project_(std::move(project)), undoManager_(undoManager), repository_(repository), factory_(factory),
      view_(view), listener_(project_, [this] { setUnsavedChanges(true); })
{
    view_.setWindowTitle(windowTitle());
}

void DocumentController::newProject()
{
    ifChangesSaved([this] { replaceProject(factory_.createDefault(), std::nullopt); });
}

void DocumentController::open()
{
    ifChangesSaved(
        [this]
        {
            view_.chooseFileToOpen(
                [this](const std::optional<std::filesystem::path>& file)
                {
                    if (file)
                        openFile(*file);
                });
        });
}

void DocumentController::save(std::function<void(bool saved)> onDone)
{
    if (!currentFile_)
    {
        saveAs(std::move(onDone));
        return;
    }
    onDone(saveTo(*currentFile_));
}

void DocumentController::saveAs(std::function<void(bool saved)> onDone)
{
    view_.chooseFileToSave(
        displayName() + kFileExtension,
        [this, onDone = std::move(onDone)](const std::optional<std::filesystem::path>& file)
        {
            if (!file)
            {
                onDone(false);
                return;
            }
            onDone(saveTo(withFileExtension(*file)));
        });
}

void DocumentController::requestClose(std::function<void()> onClose)
{
    ifChangesSaved(std::move(onClose));
}

void DocumentController::reportMissingGlobalKitSamples(const std::vector<io::MissingSample>& missing)
{
    if (!missing.empty())
        view_.showMessage("Fehlende Samples im Programm-Kit", missingSamplesMessage(missing));
}

void DocumentController::undo()
{
    undoManager_.undo();
}

void DocumentController::redo()
{
    undoManager_.redo();
}

bool DocumentController::canUndo() const
{
    return undoManager_.canUndo();
}

bool DocumentController::canRedo() const
{
    return undoManager_.canRedo();
}

std::string DocumentController::windowTitle() const
{
    return (unsavedChanges_ ? kUnsavedMarker : "") + displayName() + " – " + kApplicationName;
}

void DocumentController::ifChangesSaved(std::function<void()> action)
{
    if (!unsavedChanges_)
    {
        action();
        return;
    }
    view_.askToSaveChanges(displayName(),
                           [this, action = std::move(action)](SaveChangesChoice choice)
                           {
                               if (choice == SaveChangesChoice::discard)
                                   action();
                               else if (choice == SaveChangesChoice::save)
                                   save(
                                       [action](bool saved)
                                       {
                                           if (saved)
                                               action();
                                       });
                           });
}

void DocumentController::openFile(const std::filesystem::path& file)
{
    const auto result = repository_.load(file);
    if (result.error != io::ProjectFileError::none)
    {
        view_.showMessage("Öffnen fehlgeschlagen",
                          "„" + io::utf8FromPath(file.filename()) + "“ " + loadErrorMessage(result.error));
        return;
    }
    replaceProject(result.project, file);
    if (!result.missingSamples.empty())
        view_.showMessage("Fehlende Samples", missingSamplesMessage(result.missingSamples));
}

bool DocumentController::saveTo(const std::filesystem::path& file)
{
    if (repository_.save(project_, file) != io::ProjectFileError::none)
    {
        view_.showMessage("Speichern fehlgeschlagen",
                          "Das Projekt konnte nicht als „" + io::utf8FromPath(file) +
                              "“ gespeichert werden.");
        return false;
    }
    currentFile_ = file;
    setUnsavedChanges(false);
    return true;
}

void DocumentController::replaceProject(const juce::ValueTree& newProject,
                                        std::optional<std::filesystem::path> file)
{
    project_.copyPropertiesAndChildrenFrom(newProject, nullptr);
    undoManager_.clearUndoHistory();
    currentFile_ = std::move(file);
    setUnsavedChanges(false);
}

void DocumentController::setUnsavedChanges(bool unsavedChanges)
{
    unsavedChanges_ = unsavedChanges;
    view_.setWindowTitle(windowTitle());
}

std::string DocumentController::displayName() const
{
    if (currentFile_)
        return io::utf8FromPath(currentFile_->stem());
    return model::Project{project_, nullptr}.name();
}

std::filesystem::path DocumentController::withFileExtension(std::filesystem::path file)
{
    file.replace_extension(kFileExtension);
    return file;
}

std::string DocumentController::loadErrorMessage(io::ProjectFileError error)
{
    switch (error)
    {
    case io::ProjectFileError::unreadable:
        return "konnte nicht gelesen werden.";
    case io::ProjectFileError::newerFormatVersion:
        return "wurde mit einer neueren Version von Drum Programmer gespeichert. Bitte aktualisiere das "
               "Programm.";
    default:
        return "ist keine Drum-Programmer-Projektdatei.";
    }
}

std::string DocumentController::missingSamplesMessage(const std::vector<io::MissingSample>& missing)
{
    std::string message =
        "Diese Sample-Dateien wurden nicht gefunden. Die Slots sind als „kein Sample“ markiert, "
        "bis du eine Datei zuweist:\n\n";
    for (const auto& sample : missing)
        message += sample.slotName + ": " + sample.filePath + "\n";
    return message;
}

} // namespace drumprog::ui
