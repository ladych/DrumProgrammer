#pragma once

#include "io/MidiFileWriter.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::io
{

/// The notes of a Standard MIDI File, in the file's own resolution.
struct MidiFileContent
{
    std::string name;             ///< first track name in the file, UTF-8 as stored; empty if there is none
    int ticksPerQuarter = 0;      ///< division of the file
    double bpm = 0.0;             ///< first tempo in the file; 0 if it has none
    std::int64_t lengthTicks = 0; ///< latest end of track
    std::vector<MidiNote> notes;  ///< sorted by start
};

/// Reads a Standard MIDI File of type 0 or 1 (F-MI-03). All tracks are merged. If the file has notes on
/// channel 10 (GM drums), only those are taken, otherwise the notes of every channel. A note lasts from its
/// note-on to the next note-off of the same note and channel; one that is never switched off ends at the
/// end of its track. Running status, system exclusive and meta events are understood; of the meta events the
/// first track name and the first valid tempo are kept. Returns std::nullopt for anything that is not such a
/// file: wrong header, type 2, SMPTE division or truncated data.
[[nodiscard]] std::optional<MidiFileContent> readMidiFile(const std::string& bytes);

} // namespace drumprog::io
