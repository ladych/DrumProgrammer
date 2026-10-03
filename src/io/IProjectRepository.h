#pragma once

#include "io/ProjectSerializer.h"

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <string>
#include <vector>

namespace drumprog::io
{

struct MissingSample
{
    std::string slotName;
    std::string filePath;
};

struct LoadResult
{
    juce::ValueTree project;
    ProjectFileError error = ProjectFileError::none;
    std::vector<MissingSample> missingSamples;
};

/// Reads and writes .dpp project files.
class IProjectRepository
{
public:
    virtual ~IProjectRepository() = default;

    [[nodiscard]] virtual LoadResult load(const std::filesystem::path& file) = 0;
    [[nodiscard]] virtual ProjectFileError save(const juce::ValueTree& project,
                                                const std::filesystem::path& file) = 0;

protected:
    IProjectRepository() = default;
    IProjectRepository(const IProjectRepository&) = default;
    IProjectRepository(IProjectRepository&&) = default;
    IProjectRepository& operator=(const IProjectRepository&) = default;
    IProjectRepository& operator=(IProjectRepository&&) = default;
};

} // namespace drumprog::io
