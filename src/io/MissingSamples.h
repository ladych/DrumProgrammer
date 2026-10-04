#pragma once

#include "io/IFileSystem.h"
#include "io/IProjectRepository.h"
#include "model/Project.h"

#include <vector>

namespace drumprog::io
{

/// Marks the slots of the kit whose sample file does not exist (F-PJ-03), so they show "kein Sample"
/// until a file is assigned, and returns them in kit order.
[[nodiscard]] std::vector<MissingSample> markMissingSamples(const model::Kit& kit,
                                                            const IFileSystem& fileSystem);

} // namespace drumprog::io
