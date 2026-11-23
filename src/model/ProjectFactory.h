#pragma once

#include "engine/DrumGizmoMapping.h"
#include "model/IIdGenerator.h"

#include <juce_data_structures/juce_data_structures.h>

#include <map>
#include <string>

namespace drumprog::model
{

/// Creates new projects (F-PJ-01): 120 BPM, 4/4 and one empty pattern of two bars. New projects have
/// no own kit and play with the global kit (program setting), which starts as the GM default kit with
/// notes 35-59 (F-SE-04). The kit comes from engine::makeGmDefaultKit(), the program's only GM table.
class ProjectFactory
{
public:
    static constexpr double kDefaultBpm = 120.0;
    static constexpr int kDefaultPatternBars = 2;

    explicit ProjectFactory(IIdGenerator& idGenerator);

    [[nodiscard]] juce::ValueTree createDefault() const;
    [[nodiscard]] static juce::ValueTree createDefaultKit();
    /// The GM default kit as a DrumGizmo kit (F-DG-04): drumkitFile is the absolute UTF-8 path of
    /// drumkit.xml, each slot plays the instrument and choke group of its mapping (F-DG-07, F-DG-08).
    [[nodiscard]] static juce::ValueTree
    createDrumGizmoKit(const std::string& drumkitFile,
                       const std::map<int, engine::DrumGizmoSlotMapping>& mapping);

private:
    IIdGenerator& idGenerator_;
};

} // namespace drumprog::model
