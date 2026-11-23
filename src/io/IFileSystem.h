#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::io
{

/// File access behind an interface so logic can be tested without touching disk (E-06).
class IFileSystem
{
public:
    virtual ~IFileSystem() = default;

    [[nodiscard]] virtual std::optional<std::string> readText(const std::filesystem::path& file) const = 0;
    virtual bool writeText(const std::filesystem::path& file, const std::string& text) = 0;
    virtual bool createDirectories(const std::filesystem::path& directory) = 0;
    [[nodiscard]] virtual bool exists(const std::filesystem::path& file) const = 0;
    /// Replaces an existing target file.
    virtual bool rename(const std::filesystem::path& from, const std::filesystem::path& to) = 0;
    /// Deletes the file; true if it is gone afterwards, also if it did not exist.
    virtual bool remove(const std::filesystem::path& file) = 0;
    /// The regular files directly inside the directory, sorted; empty if it cannot be read.
    [[nodiscard]] virtual std::vector<std::filesystem::path>
    listFiles(const std::filesystem::path& directory) const = 0;

protected:
    IFileSystem() = default;
    IFileSystem(const IFileSystem&) = default;
    IFileSystem(IFileSystem&&) = default;
    IFileSystem& operator=(const IFileSystem&) = default;
    IFileSystem& operator=(IFileSystem&&) = default;
};

} // namespace drumprog::io
