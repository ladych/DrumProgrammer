#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace drumprog::io
{

struct MidiNote
{
    int note = 0;
    std::int64_t startTick = 0;
    std::int64_t lengthTicks = 0;
    int velocity = 0; ///< 1-127
};

/// One pattern as it goes into a MIDI file; ticks in the project's resolution.
struct MidiPattern
{
    std::string name; ///< UTF-8, becomes the track name
    double bpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4; ///< power of two
    int ticksPerQuarter = 0;
    std::int64_t lengthTicks = 0;
    std::vector<MidiNote> notes;
};

/// Standard MIDI File type 0 (F-MI-01): one track with name, time signature and tempo, the notes on
/// channel 10 (GM drums) and the end of track at the pattern end, so a DAW keeps the pattern length.
/// The division is the project's ticks per quarter (960), so ticks are written unchanged. Notes are
/// cut at the pattern end; notes starting at or after it are left out. Returns the file's bytes.
[[nodiscard]] std::string writeMidiFile(const MidiPattern& pattern);

} // namespace drumprog::io
