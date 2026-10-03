#pragma once

#include "io/IFileSystem.h"

namespace drumprog::io
{

/// IFileSystem on top of std::filesystem and file streams.
class StdFileSystem final : public IFileSystem
{
public:
    [[nodiscard]] std::optional<std::string> readText(const std::filesystem::path& file) const override;
    bool writeText(const std::filesystem::path& file, const std::string& text) override;
    bool createDirectories(const std::filesystem::path& directory) override;
};

} // namespace drumprog::io
