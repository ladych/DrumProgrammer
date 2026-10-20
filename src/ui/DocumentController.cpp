#include "ui/DocumentController.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "model/Project.h"

#include <algorithm>
#include <cctype>
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

void DocumentController::open(const std::filesystem::path& file)
{
    ifChangesSaved([this, file] { openFile(file); });
}

std::optional<std::filesystem::path>
DocumentController::projectFileIn(const std::vector<std::string>& arguments)
{
    const std::string extension = kFileExtension;
    for (auto argument : arguments)
    {
        if (argument.size() >= 2 && argument.front() == '"' && argument.back() == '"')
            argument = argument.substr(1, argument.size() - 2);
        if (argument.size() <= extension.size())
            continue;
        auto ending = argument.substr(argument.size() - extension.size());
        std::ranges::transform(ending, ending.begin(), [](unsigned char c) { return std::tolower(c); });
        if (ending == extension)
            return io::pathFromUtf8(argument);
    }
    return std::nullopt;
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
        view_.showMessage(i18n::tr("Missing samples in the program kit"), missingSamplesMessage(missing));
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
        view_.showMessage(i18n::tr("Opening failed"),
                          loadErrorMessage(result.error, io::utf8FromPath(file.filename())));
        return;
    }
    replaceProject(result.project, file);
    reportMissingSamples(result.missingSamples);
}

void DocumentController::restoreBackup(const io::LoadResult& backup,
                                       std::optional<std::filesystem::path> file)
{
    replaceProject(backup.project, std::move(file), true);
    reportMissingSamples(backup.missingSamples);
}

void DocumentController::reportMissingSamples(const std::vector<io::MissingSample>& missing)
{
    if (!missing.empty())
        view_.showMessage(i18n::tr("Missing samples"), missingSamplesMessage(missing));
}

bool DocumentController::saveTo(const std::filesystem::path& file)
{
    if (repository_.save(project_, file) != io::ProjectFileError::none)
    {
        view_.showMessage(i18n::tr("Saving failed"),
                          i18n::tr("The project could not be saved as \"%1\".", io::utf8FromPath(file)));
        return false;
    }
    currentFile_ = file;
    setUnsavedChanges(false);
    return true;
}

void DocumentController::replaceProject(const juce::ValueTree& newProject,
                                        std::optional<std::filesystem::path> file,
                                        bool unsavedChanges)
{
    project_.copyPropertiesAndChildrenFrom(newProject, nullptr);
    undoManager_.clearUndoHistory();
    currentFile_ = std::move(file);
    setUnsavedChanges(unsavedChanges);
}

void DocumentController::setUnsavedChanges(bool unsavedChanges)
{
    unsavedChanges_ = unsavedChanges;
    view_.setWindowTitle(windowTitle());
    if (!unsavedChanges)
        onNoUnsavedChanges_();
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

std::string DocumentController::loadErrorMessage(io::ProjectFileError error, const std::string& fileName)
{
    switch (error)
    {
    case io::ProjectFileError::unreadable:
        return i18n::tr("\"%1\" could not be read.", fileName);
    case io::ProjectFileError::newerFormatVersion:
        return i18n::tr(
            "\"%1\" was saved with a newer version of Drum Programmer. Please update the program.", fileName);
    default:
        return i18n::tr("\"%1\" is not a Drum Programmer project file.", fileName);
    }
}

std::string DocumentController::missingSamplesMessage(const std::vector<io::MissingSample>& missing)
{
    std::string message =
        i18n::tr("These sample files were not found. Their slots are marked as \"no sample\" "
                 "until you assign a file:\n\n");
    for (const auto& sample : missing)
        message += sample.slotName + ": " + sample.filePath + "\n";
    return message;
}

} // namespace drumprog::ui
