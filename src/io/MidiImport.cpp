#include "io/MidiImport.h"

#include <algorithm>
#include <optional>

namespace drumprog::io
{
namespace
{

/// The slot's GM note for a MIDI note: the slot with that GM note, otherwise the slot it triggers.
std::optional<int> slotNoteFor(const model::Kit& kit, int midiNote)
{
    if (kit.findSlot(midiNote))
        return midiNote;
    for (int index = 0; index < kit.numSlots(); ++index)
    {
        const auto slot = kit.slot(index);
        if (slot.midiNote() == midiNote)
            return slot.gmNote();
    }
    return std::nullopt;
}

/// File ticks to project ticks, rounded to the nearest tick.
class TickConverter
{
public:
    TickConverter(std::int64_t fileTicks, std::int64_t projectTicks)
        : fileTicks_(fileTicks), projectTicks_(projectTicks)
    {
    }

    [[nodiscard]] std::int64_t operator()(std::int64_t tick) const
    {
        return (tick * projectTicks_ + fileTicks_ / 2) / fileTicks_;
    }

private:
    std::int64_t fileTicks_;
    std::int64_t projectTicks_;
};

/// Adds the note to the pattern, or counts it as unmatched if no slot plays it.
void addNote(ImportedPattern& imported,
             const MidiNote& note,
             const model::Kit& kit,
             const TickConverter& convert)
{
    const auto slotNote = slotNoteFor(kit, note.note);
    if (!slotNote)
    {
        ++imported.numUnmatched;
        imported.unmatchedNotes.push_back(note.note);
        return;
    }
    model::NoteData data;
    data.slotNote = *slotNote;
    data.startTick = convert(note.startTick);
    data.lengthTicks = std::max<std::int64_t>(convert(note.startTick + note.lengthTicks) - data.startTick, 1);
    data.velocity = note.velocity;
    data.origin = model::NoteOrigin::live;
    imported.notes.push_back(data);
}

} // namespace

ImportedPattern
importedPatternOf(const MidiFileContent& file, const model::Project& project, const model::Kit& kit)
{
    const TickConverter convert{file.ticksPerQuarter, project.ticksPerQuarter()};
    ImportedPattern imported;
    std::int64_t endTick = convert(file.lengthTicks);
    for (const auto& note : file.notes)
    {
        endTick = std::max(endTick, convert(note.startTick) + 1);
        addNote(imported, note, kit, convert);
    }
    std::ranges::sort(imported.unmatchedNotes);
    const auto duplicates = std::ranges::unique(imported.unmatchedNotes);
    imported.unmatchedNotes.erase(duplicates.begin(), duplicates.end());

    const auto barTicks = project.ticksPerBar();
    imported.lengthBars = static_cast<int>(std::max<std::int64_t>((endTick + barTicks - 1) / barTicks, 1));
    return imported;
}

} // namespace drumprog::io
