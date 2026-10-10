#pragma once

#include "engine/FakeAudioFileStream.h"
#include "ui/IBackingTrackLoader.h"

#include <filesystem>
#include <map>
#include <memory>
#include <utility>

namespace drumprog::ui
{

/// Opens the files added with add() as ramps of the given rate and length.
class FakeBackingTrackLoader final : public IBackingTrackLoader
{
public:
    void add(const std::filesystem::path& file, double sampleRate, int frames)
    {
        files_[file] = {sampleRate, frames};
    }

    [[nodiscard]] std::unique_ptr<engine::IAudioFileStream> open(const std::filesystem::path& file) override
    {
        ++opened;
        const auto found = files_.find(file);
        if (found == files_.end())
            return nullptr;
        return engine::FakeAudioFileStream::ramp(found->second.first, found->second.second);
    }

    int opened = 0;

private:
    std::map<std::filesystem::path, std::pair<double, int>> files_;
};

} // namespace drumprog::ui
