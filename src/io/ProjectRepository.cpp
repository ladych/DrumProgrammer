#include "io/ProjectRepository.h"

#include "io/MissingSamples.h"
#include "model/Project.h"

#include <utility>

namespace drumprog::io
{

ProjectRepository::ProjectRepository(IFileSystem& fileSystem) : fileSystem_(fileSystem) {}

LoadResult ProjectRepository::load(const std::filesystem::path& file)
{
    const auto text = fileSystem_.readText(file);
    if (!text)
        return {{}, ProjectFileError::unreadable, {}};

    auto parsed = ProjectSerializer::fromXml(*text, file.parent_path());
    if (parsed.error != ProjectFileError::none)
        return {{}, parsed.error, {}};

    auto missing = markMissingSamples(model::Project{parsed.project, nullptr}.kit(), fileSystem_);
    return {parsed.project, ProjectFileError::none, std::move(missing)};
}

ProjectFileError ProjectRepository::save(const juce::ValueTree& project, const std::filesystem::path& file)
{
    auto tempFile = file;
    tempFile += ".tmp";
    const auto text = ProjectSerializer::toXml(project, file.parent_path());
    if (!fileSystem_.writeText(tempFile, text) || !fileSystem_.rename(tempFile, file))
        return ProjectFileError::writeFailed;
    return ProjectFileError::none;
}

} // namespace drumprog::io
