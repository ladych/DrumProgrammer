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
};

} // namespace drumprog::io
