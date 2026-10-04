#include "io/MissingSamples.h"

#include "io/Utf8Path.h"
#include "model/ModelIds.h"

namespace drumprog::io
{

std::vector<MissingSample> markMissingSamples(const model::Kit& kit, const IFileSystem& fileSystem)
{
    std::vector<MissingSample> missing;
    for (int index = 0; index < kit.numSlots(); ++index)
    {
        const auto slot = kit.slot(index);
        const auto filePath = slot.filePath();
        if (filePath.empty())
            continue;
        const auto path = pathFromUtf8(filePath);
        if (fileSystem.exists(path))
            continue;

        missing.push_back({slot.name(), filePath});
        auto slotTree = slot.tree();
        slotTree.setProperty(model::ids::sampleMissing, true, nullptr);
    }
    return missing;
}

} // namespace drumprog::io
