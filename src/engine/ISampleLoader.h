#pragma once

#include "engine/SampleBuffer.h"

#include <filesystem>
#include <optional>

namespace drumprog::engine
{

/// Decodes an audio file into RAM (E-06). The JUCE implementation decides which
/// formats are supported, so AIFF/FLAC can be added without touching the engine (F-SE-01).
class ISampleLoader
{
public:
    virtual ~ISampleLoader() = default;

    /// Returns nothing if the file is missing or cannot be decoded.
    [[nodiscard]] virtual std::optional<SampleBuffer> load(const std::filesystem::path& file) = 0;

protected:
    ISampleLoader() = default;
    ISampleLoader(const ISampleLoader&) = default;
    ISampleLoader(ISampleLoader&&) = default;
    ISampleLoader& operator=(const ISampleLoader&) = default;
    ISampleLoader& operator=(ISampleLoader&&) = default;
};

} // namespace drumprog::engine
