#include "io/SongMidiExport.h"

#include "model/SongLayout.h"

#include <algorithm>

namespace drumprog::io
{

MidiPattern midiSongOf(const model::Project& project)
{
    const auto signature = project.timeSignature();
    const auto blocks = model::layoutSong(project);
    MidiPattern midi;
    midi.name = project.name();
    midi.bpm = project.bpm();
    midi.timeSigNumerator = signature.numerator;
    midi.timeSigDenominator = signature.denominator;
    midi.ticksPerQuarter = project.ticksPerQuarter();
    midi.lengthTicks = model::songLengthTicks(blocks);
    for (const auto& block : blocks)
    {
        const auto pattern = project.pattern(block.patternIndex);
        for (int index = 0; index < pattern.numNotes(); ++index)
        {
            const auto note = pattern.note(index).data();
            if (note.startTick >= block.playedTicks)
                continue;
            MidiNote midiNote;
            midiNote.note = note.slotNote;
            midiNote.startTick = block.startTick + note.startTick;
            midiNote.lengthTicks = std::min(note.lengthTicks, block.playedTicks - note.startTick);
            midiNote.velocity = note.velocity;
            midi.notes.push_back(midiNote);
        }
    }
    return midi;
}

} // namespace drumprog::io
