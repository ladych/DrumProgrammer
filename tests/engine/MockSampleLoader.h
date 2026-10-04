#pragma once

#include "engine/ISampleLoader.h"

#include <gmock/gmock.h>

namespace drumprog::engine
{

class MockSampleLoader : public ISampleLoader
{
public:
    MOCK_METHOD(std::optional<SampleBuffer>, load, (const std::filesystem::path& file), (override));
};

} // namespace drumprog::engine
