#pragma once

#include "engine/ProjectSnapshot.h"
#include "model/Project.h"

#include <memory>

namespace drumprog::model
{

/// Builds the immutable audio-thread view of a project: notes and song blocks sorted by time,
/// slot and pattern references resolved to indices. Notes of unknown slots and blocks of unknown
/// patterns are left out.
class SnapshotBuilder
{
public:
    [[nodiscard]] static std::unique_ptr<const engine::ProjectSnapshot> build(const Project& project);

private:
    [[nodiscard]] static std::vector<engine::SlotSnapshot> buildSlots(const Kit& kit);
    [[nodiscard]] static engine::PatternSnapshot
    buildPattern(const Pattern& pattern, const Kit& kit, std::int64_t ticksPerBar);
    [[nodiscard]] static std::vector<engine::SongEntrySnapshot> buildSong(const Project& project);
};

} // namespace drumprog::model
