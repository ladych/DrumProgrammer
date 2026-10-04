#pragma once

#include "io/IFileSystem.h"
#include "io/IProjectRepository.h"

namespace drumprog::io
{

/// .dpp files on top of IFileSystem.
///
/// Saving writes a temporary file next to the target and then renames it, so a failed write never
/// destroys the previous version (Q-10). Loading marks slots whose sample file does not exist and
/// reports them instead of failing (F-PJ-03).
class ProjectRepository final : public IProjectRepository
{
public:
    explicit ProjectRepository(IFileSystem& fileSystem);

    [[nodiscard]] LoadResult load(const std::filesystem::path& file) override;
    [[nodiscard]] ProjectFileError save(const juce::ValueTree& project,
                                        const std::filesystem::path& file) override;

private:
    [[nodiscard]] std::vector<MissingSample> markMissingSamples(juce::ValueTree& project) const;

    IFileSystem& fileSystem_;
};

} // namespace drumprog::io
