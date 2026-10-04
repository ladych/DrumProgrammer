#pragma once

#include "model/IIdGenerator.h"

#include <juce_data_structures/juce_data_structures.h>

namespace drumprog::model
{

/// Creates new projects (F-PJ-01): GM default kit with notes 35-59 (F-SE-04), 120 BPM, 4/4 and one
/// empty pattern of two bars. The kit comes from engine::makeGmDefaultKit(), the program's only GM
/// table.
class ProjectFactory
{
public:
    static constexpr double kDefaultBpm = 120.0;
    static constexpr int kDefaultPatternBars = 2;

    explicit ProjectFactory(IIdGenerator& idGenerator);

    [[nodiscard]] juce::ValueTree createDefault() const;

private:
    [[nodiscard]] static juce::ValueTree createDefaultKit();

    IIdGenerator& idGenerator_;
};

} // namespace drumprog::model
