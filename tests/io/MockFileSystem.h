#pragma once

#include "io/IFileSystem.h"

#include <gmock/gmock.h>

namespace drumprog::io
{

class MockFileSystem : public IFileSystem
{
public:
    MOCK_METHOD(std::optional<std::string>, readText, (const std::filesystem::path& file), (const, override));
    MOCK_METHOD(bool, writeText, (const std::filesystem::path& file, const std::string& text), (override));
    MOCK_METHOD(bool, createDirectories, (const std::filesystem::path& directory), (override));
    MOCK_METHOD(bool, exists, (const std::filesystem::path& file), (const, override));
    MOCK_METHOD(bool,
                rename,
                (const std::filesystem::path& from, const std::filesystem::path& to),
                (override));
    MOCK_METHOD(bool, remove, (const std::filesystem::path& file), (override));
    MOCK_METHOD(std::vector<std::filesystem::path>,
                listFiles,
                (const std::filesystem::path& directory),
                (const, override));
};

} // namespace drumprog::io
