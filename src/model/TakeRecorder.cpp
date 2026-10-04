#include "model/TakeRecorder.h"

#include "engine/TempoMath.h"
#include "model/Project.h"

#include <algorithm>
#include <utility>

namespace drumprog::model
{
namespace
{

constexpr int kSixteenthsPerQuarter = 4;

} // namespace

TakeRecorder::TakeRecorder(juce::ValueTree project, juce::UndoManager& undoManager)
    : project_(std::move(project)), undoManager_(undoManager)
{
}

bool TakeRecorder::begin(int patternIndex, RecordMode mode)
{
    const Project project{project_, &undoManager_};
    if (patternIndex < 0 || patternIndex >= project.numPatterns())
        return false;
    undoManager_.beginNewTransaction(kUndoName);
    auto pattern = project.pattern(patternIndex);
    patternId_ = pattern.id();
    if (mode == RecordMode::replace)
        for (int index = pattern.numNotes() - 1; index >= 0; --index)
            pattern.removeNote(index);
    return true;
}

void TakeRecorder::add(int slotIndex, int velocity, std::int64_t tick)
{
    if (!patternId_)
        return;
    const Project project{project_, &undoManager_};
    auto pattern = project.findPattern(*patternId_);
    const auto kit = project.kit();
    if (!pattern || slotIndex < 0 || slotIndex >= kit.numSlots())
        return;
    // Someone else's change in between (e.g. a kit edit) started its own undo step.
    if (undoManager_.getCurrentTransactionName() != kUndoName)
        undoManager_.beginNewTransaction(kUndoName);
    const std::int64_t length = pattern->lengthBars() * project.ticksPerBar();
    const std::int64_t start = engine::wrapTick(tick, length);
    const std::int64_t sixteenth = project.ticksPerQuarter() / kSixteenthsPerQuarter;
    pattern->addNote({.slotNote = kit.slot(slotIndex).gmNote(),
                      .startTick = start,
                      .lengthTicks = std::min(sixteenth, length - start),
                      .velocity = velocity,
                      .origin = NoteOrigin::live});
}

void TakeRecorder::end()
{
    if (!patternId_)
        return;
    patternId_.reset();
    undoManager_.beginNewTransaction();
}

} // namespace drumprog::model
