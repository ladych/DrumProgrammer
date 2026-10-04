#include "model/SnapshotBuilder.h"

#include "io/Utf8Path.h"
#include "model/ModelIds.h"

#include <algorithm>
#include <cmath>

namespace drumprog::model
{
namespace
{

std::filesystem::path sampleFileOf(const SampleSlot& slot)
{
    if (!slot.hasSample())
        return {};
    return io::pathFromUtf8(slot.filePath());
}

} // namespace

std::unique_ptr<const engine::ProjectSnapshot> SnapshotBuilder::build(const Project& project, const Kit& kit)
{
    auto snapshot = std::make_unique<engine::ProjectSnapshot>();
    const auto signature = project.timeSignature();
    snapshot->bpm = project.bpm();
    snapshot->timeSigNumerator = signature.numerator;
    snapshot->timeSigDenominator = signature.denominator;
    snapshot->ticksPerQuarter = project.ticksPerQuarter();

    for (int index = 0; index < project.numPatterns(); ++index)
        snapshot->patterns.push_back(buildPattern(project.pattern(index), kit, project.ticksPerBar()));
    snapshot->song = buildSong(project);

    const auto mix = project.mix();
    snapshot->mix = {mix.backingGain(), mix.drumsGain(), mix.masterGain()};
    return snapshot;
}

engine::KitDescription SnapshotBuilder::buildKit(const Kit& kit)
{
    engine::KitDescription description;
    for (int index = 0; index < kit.numSlots(); ++index)
    {
        const auto slot = kit.slot(index);
        description.push_back({.gmNote = slot.gmNote(),
                               .midiNote = slot.midiNote(),
                               .name = slot.name(),
                               .sampleFile = sampleFileOf(slot),
                               .gain = static_cast<float>(slot.gain()),
                               .pitchSemitones = static_cast<int>(std::lround(slot.pitch())),
                               .chokeGroup = slot.chokeGroup(),
                               .coreSlot = engine::isCoreGmNote(slot.gmNote())});
    }
    return description;
}

engine::PatternSnapshot
SnapshotBuilder::buildPattern(const Pattern& pattern, const Kit& kit, std::int64_t ticksPerBar)
{
    engine::PatternSnapshot snapshot;
    snapshot.lengthTicks = pattern.lengthBars() * ticksPerBar;
    for (int index = 0; index < pattern.numNotes(); ++index)
    {
        const auto note = pattern.note(index).data();
        if (const auto slot = kit.findSlot(note.slotNote))
        {
            const int slotIndex = kit.tree().indexOf(slot->tree());
            snapshot.notes.push_back({slotIndex, note.startTick, note.lengthTicks, note.velocity});
        }
    }
    std::ranges::stable_sort(snapshot.notes, {}, &engine::NoteSnapshot::startTick);
    return snapshot;
}

std::vector<engine::SongEntrySnapshot> SnapshotBuilder::buildSong(const Project& project)
{
    std::vector<engine::SongEntrySnapshot> entries;
    const auto song = project.song();
    const auto patterns = project.tree().getChildWithName(ids::patterns);
    for (int index = 0; index < song.numEntries(); ++index)
    {
        const auto entry = song.entry(index);
        if (const auto pattern = project.findPattern(entry.patternId()))
            entries.push_back({patterns.indexOf(pattern->tree()), entry.startBar() * project.ticksPerBar()});
    }
    std::ranges::stable_sort(entries, {}, &engine::SongEntrySnapshot::startTick);
    return entries;
}

} // namespace drumprog::model
