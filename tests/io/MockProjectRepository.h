#pragma once

#include "io/IProjectRepository.h"

#include <gmock/gmock.h>

namespace drumprog::io
{

class MockProjectRepository : public IProjectRepository
{
public:
    MOCK_METHOD(LoadResult, load, (const std::filesystem::path& file), (override));
    MOCK_METHOD(ProjectFileError,
                save,
                (const juce::ValueTree& project, const std::filesystem::path& file),
                (override));
};

} // namespace drumprog::io
