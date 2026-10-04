#pragma once

#include <juce_data_structures/juce_data_structures.h>

/// Type and property names of the project ValueTree (Pflichtenheft chapter 4). They are also the
/// element and attribute names in the .dpp file, so renaming one breaks existing project files.
namespace drumprog::model::ids
{

// Tree types
inline const juce::Identifier project{"PROJECT"};
inline const juce::Identifier kit{"KIT"};
inline const juce::Identifier slot{"SLOT"};
inline const juce::Identifier patterns{"PATTERNS"};
inline const juce::Identifier pattern{"PATTERN"};
inline const juce::Identifier note{"NOTE"};
inline const juce::Identifier song{"SONG"};
inline const juce::Identifier songEntry{"ENTRY"};
inline const juce::Identifier backingTrack{"BACKING_TRACK"};
inline const juce::Identifier mix{"MIX"};

// Properties
inline const juce::Identifier formatVersion{"formatVersion"};
inline const juce::Identifier name{"name"};
inline const juce::Identifier bpm{"bpm"};
inline const juce::Identifier timeSigNumerator{"timeSigNumerator"};
inline const juce::Identifier timeSigDenominator{"timeSigDenominator"};
inline const juce::Identifier ppq{"ppq"};
inline const juce::Identifier gmNote{"gmNote"};
inline const juce::Identifier midiNote{"midiNote"};
inline const juce::Identifier filePath{"filePath"};
inline const juce::Identifier gain{"gain"};
inline const juce::Identifier pitch{"pitch"};
inline const juce::Identifier chokeGroup{"chokeGroup"};
inline const juce::Identifier id{"id"};
inline const juce::Identifier colour{"colour"};
inline const juce::Identifier lengthBars{"lengthBars"};
inline const juce::Identifier slotNote{"slotNote"};
inline const juce::Identifier startTick{"startTick"};
inline const juce::Identifier lengthTicks{"lengthTicks"};
inline const juce::Identifier velocity{"velocity"};
inline const juce::Identifier origin{"origin"};
inline const juce::Identifier patternId{"patternId"};
inline const juce::Identifier startBar{"startBar"};
inline const juce::Identifier offsetSamples{"offsetSamples"};
inline const juce::Identifier backingGain{"backingGain"};
inline const juce::Identifier drumsGain{"drumsGain"};
inline const juce::Identifier masterGain{"masterGain"};

/// Runtime-only marker for a slot whose sample file was missing when the project was loaded
/// (F-PJ-03). It is never written to the .dpp file.
inline const juce::Identifier sampleMissing{"sampleMissing"};

} // namespace drumprog::model::ids
