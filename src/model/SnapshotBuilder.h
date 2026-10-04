#pragma once

#include "engine/KitDescription.h"
#include "engine/ProjectSnapshot.h"
#include "model/Project.h"

#include <memory>

namespace drumprog::model
{

/// Builds what the engine gets from a project: the immutable audio-thread view with notes and song
/// blocks sorted by time, slot and pattern references resolved to indices (notes of unknown slots and
/// blocks of unknown patterns are left out), and the kit description the engine kit is built from. The
/// kit is the one the project plays with (Project::activeKit).
class SnapshotBuilder
{
public:
    [[nodiscard]] static std::unique_ptr<const engine::ProjectSnapshot> build(const Project& project,
                                                                              const Kit& kit);
    /// Slots in kit order; a sample that was missing when the project was loaded is left empty.
    [[nodiscard]] static engine::KitDescription buildKit(const Kit& kit);

private:
    [[nodiscard]] static engine::PatternSnapshot
    buildPattern(const Pattern& pattern, const Kit& kit, std::int64_t ticksPerBar);
    [[nodiscard]] static std::vector<engine::SongEntrySnapshot> buildSong(const Project& project);
};

} // namespace drumprog::model
