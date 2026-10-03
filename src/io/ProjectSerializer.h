#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <string>

namespace drumprog::io
{

enum class ProjectFileError
{
    none,
    unreadable,
    notAProject,
    newerFormatVersion,
    writeFailed
};

struct ParsedProject
{
    juce::ValueTree project;
    ProjectFileError error = ProjectFileError::none;
};

/// Converts the project ValueTree to and from the .dpp XML text (F-PJ-02).
///
/// In memory sample and backing-track paths are absolute; in the file they are relative to the
/// project directory with forward slashes, so projects can be moved together with their samples
/// and opened on another operating system. Paths on another drive stay absolute.
class ProjectSerializer
{
public:
    static constexpr int kFormatVersion = 1;

    [[nodiscard]] static std::string toXml(const juce::ValueTree& project,
                                           const std::filesystem::path& projectDirectory);
    [[nodiscard]] static ParsedProject fromXml(const std::string& xml,
                                               const std::filesystem::path& projectDirectory);

private:
    static void convertPaths(juce::ValueTree& project,
                             const std::filesystem::path& projectDirectory,
                             bool toRelative);
    static void convertPath(juce::ValueTree& tree,
                            const std::filesystem::path& projectDirectory,
                            bool toRelative);
    static void addMissingSections(juce::ValueTree& project);
};

} // namespace drumprog::io
