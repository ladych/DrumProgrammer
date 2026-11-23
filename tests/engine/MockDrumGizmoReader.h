#pragma once

#include "engine/DrumGizmoKit.h"

#include <gmock/gmock.h>

namespace drumprog::engine
{

class MockDrumGizmoReader : public IDrumGizmoReader
{
public:
    MOCK_METHOD(DrumGizmoReadResult,
                read,
                (const std::filesystem::path& drumkitFile, bool withSamples),
                (const, override));
};

} // namespace drumprog::engine
