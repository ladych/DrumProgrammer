#pragma once

#include "io/MidiFileReader.h"
#include "model/Project.h"

#include <vector>

namespace drumprog::io
{

/// A MIDI file as the notes of a new pattern (F-MI-03, F-MI-04).
struct ImportedPattern
{
    std::vector<model::NoteData> notes;
    int lengthBars = 1;
    /// Notes of the file without a slot in the kit, each MIDI note once, ascending.
    std::vector<int> unmatchedNotes;
    int numUnmatched = 0; ///< how many notes were left out
};

/// Converts the file's ticks to the project's resolution (rounded to the nearest tick) and gives every note
/// the slot whose GM note it is, otherwise the slot it triggers (its MIDI note). Notes without a slot are
/// left out and counted. Imported notes are not on the grid, so they get origin = live (violet). The
/// pattern lasts to the end of the file, rounded up to whole bars of the project's time signature.
[[nodiscard]] ImportedPattern
importedPatternOf(const MidiFileContent& file, const model::Project& project, const model::Kit& kit);

} // namespace drumprog::io
