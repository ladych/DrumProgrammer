#include "ui/Autosave.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"

#include <string>
#include <utility>

namespace drumprog::ui
{

Autosave::Autosave(juce::ValueTree project,
                   DocumentController& document,
                   io::IProjectRepository& repository,
                   io::IFileSystem& fileSystem,
                   Files files,
                   IDocumentView& view)
    : project_(std::move(project)), document_(document), repository_(repository), fileSystem_(fileSystem),
      files_(std::move(files)), view_(view), listener_(project_, [this] { changedSinceBackup_ = true; })
{
    document_.setOnNoUnsavedChanges([this] { discard(); });
}

void Autosave::offerRecovery()
{
    if (!fileSystem_.exists(files_.backup))
        return;
    view_.askToRestoreBackup(
        [this](bool restoreIt)
        {
            if (restoreIt)
                restore();
            else
                discard();
        });
}

void Autosave::tick(double nowSeconds)
{
    if (!intervalStart_)
        intervalStart_ = nowSeconds;
    if (nowSeconds - *intervalStart_ < kIntervalSeconds)
        return;
    intervalStart_ = nowSeconds;
    if (changedSinceBackup_)
        write();
}

void Autosave::discard()
{
    fileSystem_.remove(files_.backup);
    fileSystem_.remove(files_.source);
    changedSinceBackup_ = false;
}

void Autosave::restore()
{
    const auto backup = repository_.load(files_.backup);
    if (backup.error != io::ProjectFileError::none)
    {
        view_.showMessage(i18n::tr("Restoring failed"),
                          i18n::tr("The automatic backup could not be read and was removed."));
        discard();
        return;
    }
    std::optional<std::filesystem::path> file;
    if (const auto source = fileSystem_.readText(files_.source); source && !source->empty())
        file = io::pathFromUtf8(*source);
    document_.restoreBackup(backup, file);
    // The backup on the disk is the restored state.
    changedSinceBackup_ = false;
}

void Autosave::write()
{
    std::string source;
    if (const auto& file = document_.currentFile())
        source = io::utf8FromPath(*file);
    fileSystem_.createDirectories(files_.backup.parent_path());
    const bool written = repository_.save(project_, files_.backup) == io::ProjectFileError::none &&
                         fileSystem_.writeText(files_.source, source);
    changedSinceBackup_ = !written;
}

} // namespace drumprog::ui
