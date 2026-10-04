#include "io/DeviceSettingsStore.h"

#include <utility>

namespace drumprog::io
{

DeviceSettingsStore::DeviceSettingsStore(IFileSystem& fileSystem, std::filesystem::path settingsFile)
    : fileSystem_(fileSystem), settingsFile_(std::move(settingsFile))
{
}

std::optional<std::string> DeviceSettingsStore::load() const
{
    auto state = fileSystem_.readText(settingsFile_);
    if (!state || state->empty())
        return std::nullopt;
    return state;
}

bool DeviceSettingsStore::save(const std::string& deviceState)
{
    if (!fileSystem_.createDirectories(settingsFile_.parent_path()))
        return false;
    return fileSystem_.writeText(settingsFile_, deviceState);
}

} // namespace drumprog::io
