#pragma once

#include "io/IFileSystem.h"
#include "io/IProjectRepository.h"
#include "model/TreeChangeListener.h"
#include "ui/DocumentController.h"
#include "ui/IDocumentView.h"

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <optional>

namespace drumprog::ui
{

/// Automatic backup of unsaved changes every 5 minutes (Q-10). The backup is a project file in the settings
/// directory, next to a text file with the project file it belongs to. It is removed as soon as no changes
/// are unsaved any more and when the program ends normally, so a backup at program start was left behind
/// by a crash: offerRecovery() then asks whether to restore it. GUI thread only.
class Autosave
{
public:
    static constexpr double kIntervalSeconds = 5.0 * 60.0;

    struct Files
    {
        std::filesystem::path backup; ///< the project
        std::filesystem::path source; ///< UTF-8 path of its project file, empty for a new project
    };

    /// Takes over the DocumentController's setOnNoUnsavedChanges().
    Autosave(juce::ValueTree project,
             DocumentController& document,
             io::IProjectRepository& repository,
             io::IFileSystem& fileSystem,
             Files files,
             IDocumentView& view);

    /// At program start: asks whether to restore a backup left behind, and removes it if not.
    void offerRecovery();
    /// Call regularly, e.g. every second; nowSeconds is a monotonic time. Writes the backup when the
    /// interval has passed since the first call or the last backup and the project changed since then
    /// without being saved.
    void tick(double nowSeconds);
    /// Removes the backup, e.g. when the program ends normally.
    void discard();

private:
    void restore();
    void write();

    juce::ValueTree project_;
    DocumentController& document_;
    io::IProjectRepository& repository_;
    io::IFileSystem& fileSystem_;
    Files files_;
    IDocumentView& view_;
    std::optional<double> intervalStart_;
    /// Changed since the last backup; reset when no changes are unsaved any more.
    bool changedSinceBackup_ = false;
    model::TreeChangeListener listener_;
};

} // namespace drumprog::ui
