#pragma once

#include "io/MidiFileWriter.h"
#include "model/Project.h"

namespace drumprog::io
{

/// The song timeline as one continuous pattern (F-MI-02): every block's notes at their song position, each
/// block cut where the next one starts (as it plays, model::layoutSong), named after the project and as
/// long as the song. Notes keep the GM note of their slot like the pattern export.
[[nodiscard]] MidiPattern midiSongOf(const model::Project& project);

} // namespace drumprog::io
