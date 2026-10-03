#pragma once

#include "io/IFileSystem.h"

#include <filesystem>
#include <optional>
#include <string>

namespace drumprog::io
{

/// Persists the audio device state between runs (F-AO-04). The state itself is
/// opaque text (JUCE's AudioDeviceManager XML); this class only stores it.
class DeviceSettingsStore
{
public:
    DeviceSettingsStore(IFileSystem& fileSystem, std::filesystem::path settingsFile);

    /// Returns the last saved state, or nothing if none was saved yet.
    [[nodiscard]] std::optional<std::string> load() const;

    /// Creates the settings directory if needed; returns false if writing failed.
    bool save(const std::string& deviceState);

private:
    IFileSystem& fileSystem_;
    std::filesystem::path settingsFile_;
};

} // namespace drumprog::io
