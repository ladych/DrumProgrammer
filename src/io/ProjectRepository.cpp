#include "io/ProjectRepository.h"

#include "io/Utf8Path.h"
#include "model/ModelIds.h"
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

    auto missing = markMissingSamples(parsed.project);
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

std::vector<MissingSample> ProjectRepository::markMissingSamples(juce::ValueTree& project) const
{
    std::vector<MissingSample> missing;
    const auto kit = model::Project{project, nullptr}.kit();
    for (int index = 0; index < kit.numSlots(); ++index)
    {
        const auto slot = kit.slot(index);
        const auto filePath = slot.filePath();
        if (filePath.empty())
            continue;
        const auto path = pathFromUtf8(filePath);
        if (fileSystem_.exists(path))
            continue;

        missing.push_back({slot.name(), filePath});
        auto slotTree = slot.tree();
        slotTree.setProperty(model::ids::sampleMissing, true, nullptr);
    }
    return missing;
}

} // namespace drumprog::io
