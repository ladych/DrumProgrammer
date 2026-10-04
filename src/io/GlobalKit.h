#pragma once

#include "io/IFileSystem.h"
#include "io/IProjectRepository.h"
#include "io/SettingsStore.h"
#include "model/TreeChangeListener.h"

#include <juce_data_structures/juce_data_structures.h>

#include <vector>

namespace drumprog::io
{

/// The global kit: a program setting every project plays with unless it has its own kit. It is a KIT
/// tree like a project's kit, kept as XML with absolute sample paths in the settings directory.
/// Without a saved kit, or with an unreadable one, it starts as the GM default kit (F-SE-04).
///
/// Changes are only remembered; saveIfChanged() writes them, so dragging a slider does not write the
/// file for every step. GUI thread only.
class GlobalKit
{
public:
    GlobalKit(SettingsStore& store, const IFileSystem& fileSystem);

    /// The kit tree; it lives as long as this object, so listeners can stay attached.
    [[nodiscard]] const juce::ValueTree& tree() const noexcept { return kit_; }
    /// Samples that did not exist when the kit was loaded; their slots are marked (F-PJ-03).
    [[nodiscard]] const std::vector<MissingSample>& missingSamples() const noexcept { return missing_; }
    [[nodiscard]] bool hasUnsavedChanges() const noexcept { return changed_; }

    /// Returns false if writing failed; the changes then stay unsaved.
    bool saveIfChanged();

    [[nodiscard]] static std::string toXml(const juce::ValueTree& kit);

private:
    [[nodiscard]] static juce::ValueTree load(const SettingsStore& store);

    SettingsStore& store_;
    juce::ValueTree kit_;
    std::vector<MissingSample> missing_;
    bool changed_ = false;
    model::TreeChangeListener listener_;
};

} // namespace drumprog::io
