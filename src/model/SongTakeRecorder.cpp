#include "model/SongTakeRecorder.h"

#include "engine/TempoMath.h"
#include "model/Project.h"

#include <algorithm>
#include <string>
#include <utility>

namespace drumprog::model
{
namespace
{

constexpr int kSixteenthsPerQuarter = 4;

} // namespace

SongTakeRecorder::SongTakeRecorder(juce::ValueTree project,
                                   juce::ValueTree globalKit,
                                   juce::UndoManager& undoManager,
                                   IIdGenerator& idGenerator)
    : project_(std::move(project)), globalKit_(std::move(globalKit)), undoManager_(undoManager),
      idGenerator_(idGenerator)
{
}

void SongTakeRecorder::begin()
{
    hits_.clear();
    recording_ = true;
}

void SongTakeRecorder::add(int slotIndex, int velocity, std::int64_t songTick)
{
    const auto kit = Project{project_, nullptr}.activeKit(globalKit_);
    if (!recording_ || slotIndex < 0 || slotIndex >= kit.numSlots())
        return;
    hits_.push_back({kit.slot(slotIndex).gmNote(), velocity, songTick});
}

std::optional<int> SongTakeRecorder::end(std::int64_t startTick, std::int64_t stopTick)
{
    const bool recorded = recording_ && !hits_.empty();
    recording_ = false;
    if (!recorded)
        return std::nullopt;
    Project project{project_, &undoManager_};
    const std::int64_t barTicks = project.ticksPerBar();
    std::int64_t endTick = stopTick;
    for (const auto& hit : hits_)
        endTick = std::max(endTick, hit.tick + 1);
    const std::int64_t startBar = engine::floorDiv(std::max<std::int64_t>(startTick, 0), barTicks);
    const std::int64_t endBar = engine::floorDiv(endTick + barTicks - 1, barTicks);

    undoManager_.beginNewTransaction(kUndoName);
    const auto id = idGenerator_.next();
    auto pattern =
        project.addPattern(id, nextName(), static_cast<int>(std::max<std::int64_t>(endBar - startBar, 1)));
    writeNotes(pattern, startBar * barTicks, pattern.lengthBars() * barTicks);
    project.song().addEntry(id, static_cast<int>(startBar));
    undoManager_.beginNewTransaction();
    hits_.clear();
    return project.numPatterns() - 1;
}

void SongTakeRecorder::writeNotes(Pattern& pattern, std::int64_t startTick, std::int64_t length) const
{
    const std::int64_t sixteenth = Project{project_, nullptr}.ticksPerQuarter() / kSixteenthsPerQuarter;
    for (const auto& hit : hits_)
    {
        // Hits just before the start of the recording go to its first tick.
        const std::int64_t start = std::clamp<std::int64_t>(hit.tick - startTick, 0, length - 1);
        pattern.addNote({.slotNote = hit.slotNote,
                         .startTick = start,
                         .lengthTicks = std::min(sixteenth, length - start),
                         .velocity = hit.velocity,
                         .origin = NoteOrigin::live});
    }
}

std::string SongTakeRecorder::nextName() const
{
    const Project project{project_, nullptr};
    int number = 1;
    for (int index = 0; index < project.numPatterns(); ++index)
    {
        const auto name = project.pattern(index).name();
        if (name.starts_with(kNamePrefix))
        {
            const auto suffix = name.substr(std::string{kNamePrefix}.size());
            const int taken = juce::String{suffix}.getIntValue();
            number = std::max(number, taken + 1);
        }
    }
    return kNamePrefix + std::to_string(number);
}

} // namespace drumprog::model
