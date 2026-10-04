#pragma once

#include "io/MidiFileWriter.h"
#include "model/Project.h"

#include <filesystem>

namespace drumprog::io
{

/// The pattern with tempo, time signature and resolution of the project (F-MI-01). Notes keep the GM
/// note of their slot, so the file plays the same instruments in any GM drum map.
[[nodiscard]] MidiPattern midiPatternOf(const model::Project& project, const model::Pattern& pattern);

/// Appends ".mid" unless the file already ends in .mid or .midi (any case).
[[nodiscard]] std::filesystem::path withMidiExtension(std::filesystem::path file);

} // namespace drumprog::io
