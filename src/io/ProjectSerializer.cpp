#include "io/ProjectSerializer.h"

#include "io/Utf8Path.h"
#include "model/ModelIds.h"

namespace drumprog::io
{
namespace ids = model::ids;

namespace
{

juce::String toJuce(const std::string& utf8)
{
    return juce::String::fromUTF8(utf8.c_str());
}

std::string makeRelative(const std::filesystem::path& path, const std::filesystem::path& directory)
{
    // Empty if no relative path exists, e.g. to another drive on Windows: then the path stays as it is.
    const auto relative = path.lexically_relative(directory);
    return genericUtf8FromPath(relative.empty() ? path : relative);
}

std::string makeAbsolute(const std::filesystem::path& path, const std::filesystem::path& directory)
{
    return utf8FromPath((directory / path).lexically_normal());
}

} // namespace

std::string ProjectSerializer::toXml(const juce::ValueTree& project,
                                     const std::filesystem::path& projectDirectory)
{
    auto copy = project.createCopy();
    copy.setProperty(ids::formatVersion, kFormatVersion, nullptr);
    convertPaths(copy, projectDirectory, true);
    for (auto slot : copy.getChildWithName(ids::kit))
        slot.removeProperty(ids::sampleMissing, nullptr);
    return copy.toXmlString().toStdString();
}

ParsedProject ProjectSerializer::fromXml(const std::string& xml,
                                         const std::filesystem::path& projectDirectory)
{
    auto project = juce::ValueTree::fromXml(toJuce(xml));
    if (!project.hasType(ids::project) || !project.hasProperty(ids::formatVersion))
        return {{}, ProjectFileError::notAProject};
    if (static_cast<int>(project[ids::formatVersion]) > kFormatVersion)
        return {{}, ProjectFileError::newerFormatVersion};

    project.removeProperty(ids::formatVersion, nullptr);
    addMissingSections(project);
    convertPaths(project, projectDirectory, false);
    return {project, ProjectFileError::none};
}

void ProjectSerializer::convertPaths(juce::ValueTree& project,
                                     const std::filesystem::path& projectDirectory,
                                     bool toRelative)
{
    for (auto slot : project.getChildWithName(ids::kit))
        convertPath(slot, projectDirectory, toRelative);
    auto backingTrack = project.getChildWithName(ids::backingTrack);
    convertPath(backingTrack, projectDirectory, toRelative);
}

void ProjectSerializer::convertPath(juce::ValueTree& tree,
                                    const std::filesystem::path& projectDirectory,
                                    bool toRelative)
{
    const auto stored = tree[ids::filePath].toString().toStdString();
    if (stored.empty())
        return;
    const auto path = pathFromUtf8(stored);
    const auto converted =
        toRelative ? makeRelative(path, projectDirectory) : makeAbsolute(path, projectDirectory);
    tree.setProperty(ids::filePath, toJuce(converted), nullptr);
}

void ProjectSerializer::addMissingSections(juce::ValueTree& project)
{
    for (const auto* type : {&ids::kit, &ids::patterns, &ids::song, &ids::backingTrack, &ids::mix})
        static_cast<void>(project.getOrCreateChildWithName(*type, nullptr));
}

} // namespace drumprog::io
