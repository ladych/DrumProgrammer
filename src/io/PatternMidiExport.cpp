#include "io/PatternMidiExport.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace drumprog::io
{

MidiPattern midiPatternOf(const model::Project& project, const model::Pattern& pattern)
{
    const auto signature = project.timeSignature();
    MidiPattern midi;
    midi.name = pattern.name();
    midi.bpm = project.bpm();
    midi.timeSigNumerator = signature.numerator;
    midi.timeSigDenominator = signature.denominator;
    midi.ticksPerQuarter = project.ticksPerQuarter();
    midi.lengthTicks = pattern.lengthBars() * project.ticksPerBar();
    for (int index = 0; index < pattern.numNotes(); ++index)
    {
        const auto note = pattern.note(index).data();
        midi.notes.push_back({.note = note.slotNote,
                              .startTick = note.startTick,
                              .lengthTicks = note.lengthTicks,
                              .velocity = note.velocity});
    }
    return midi;
}

std::filesystem::path withMidiExtension(std::filesystem::path file)
{
    auto extension = file.extension().string();
    std::ranges::transform(
        extension, extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension != ".mid" && extension != ".midi")
        file += ".mid";
    return file;
}

} // namespace drumprog::io
