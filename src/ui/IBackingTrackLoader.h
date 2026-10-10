#pragma once

#include "engine/IAudioFileStream.h"

#include <filesystem>
#include <memory>

namespace drumprog::ui
{

/// Opens audio files for streaming (F-BT-01, F-BT-02); the app implements it with JUCE.
class IBackingTrackLoader
{
public:
    IBackingTrackLoader() = default;
    virtual ~IBackingTrackLoader() = default;
    IBackingTrackLoader(const IBackingTrackLoader&) = delete;
    IBackingTrackLoader& operator=(const IBackingTrackLoader&) = delete;
    IBackingTrackLoader(IBackingTrackLoader&&) = delete;
    IBackingTrackLoader& operator=(IBackingTrackLoader&&) = delete;

    /// Null if the file cannot be read as audio.
    [[nodiscard]] virtual std::unique_ptr<engine::IAudioFileStream>
    open(const std::filesystem::path& file) = 0;
};

} // namespace drumprog::ui
