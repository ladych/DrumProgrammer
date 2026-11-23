#include "io/MissingSamples.h"

#include "io/Utf8Path.h"
#include "model/ModelIds.h"

namespace drumprog::io
{

namespace
{

void markMissingDrumkit(const model::Kit& kit,
                        const IFileSystem& fileSystem,
                        std::vector<MissingSample>& missing)
{
    const auto drumkitFile = kit.drumkitFile();
    const auto path = pathFromUtf8(drumkitFile);
    if (fileSystem.exists(path))
        return;
    missing.push_back({"DrumGizmo", drumkitFile});
    auto kitTree = kit.tree();
    kitTree.setProperty(model::ids::sampleMissing, true, nullptr);
}

} // namespace

std::vector<MissingSample> markMissingSamples(const model::Kit& kit, const IFileSystem& fileSystem)
{
    std::vector<MissingSample> missing;
    // A DrumGizmo kit has no sample files of its own; its drumkit.xml is what can go missing.
    if (kit.isDrumGizmo())
        markMissingDrumkit(kit, fileSystem, missing);
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
