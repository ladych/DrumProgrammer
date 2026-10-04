#pragma once

#include "model/IIdGenerator.h"

#include <juce_data_structures/juce_data_structures.h>

namespace drumprog::model
{

/// Creates new projects (F-PJ-01): GM default kit with notes 35-59 (F-SE-04), 120 BPM, 4/4 and one
/// empty pattern of two bars.
class ProjectFactory
{
public:
    static constexpr double kDefaultBpm = 120.0;
    static constexpr int kDefaultPatternBars = 2;
    static constexpr int kFirstGmNote = 35;
    static constexpr int kLastGmNote = 59;
    /// Closed (42), pedal (44) and open hi-hat (46) choke each other (F-SE-08).
    static constexpr int kHiHatChokeGroup = 1;

    explicit ProjectFactory(IIdGenerator& idGenerator);

    [[nodiscard]] juce::ValueTree createDefault() const;

private:
    [[nodiscard]] static juce::ValueTree createDefaultKit();

    IIdGenerator& idGenerator_;
};

} // namespace drumprog::model
