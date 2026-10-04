#include "engine/EngineKit.h"

#include <utility>

namespace drumprog::engine
{
namespace
{

constexpr int kNoSlot = -1;

constexpr bool isValidNote(int midiNote) noexcept
{
    return midiNote >= 0 && midiNote < EngineKit::kNumMidiNotes;
}

} // namespace

EngineKit::EngineKit(std::vector<EngineSlot> slots) : slots_(std::move(slots))
{
    noteToSlot_.fill(kNoSlot);
    for (int i = static_cast<int>(slots_.size()) - 1; i >= 0; --i)
    {
        const int note = slots_[static_cast<std::size_t>(i)].midiNote;
        if (isValidNote(note))
            noteToSlot_.at(static_cast<std::size_t>(note)) = i;
    }
}

const EngineSlot* EngineKit::slotForNote(int midiNote) const noexcept
{
    if (!isValidNote(midiNote))
        return nullptr;
    const int index = noteToSlot_.at(static_cast<std::size_t>(midiNote));
    return index == kNoSlot ? nullptr : &slots_[static_cast<std::size_t>(index)];
}

const EngineSlot* EngineKit::slotAt(int slotIndex) const noexcept
{
    if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= slots_.size())
        return nullptr;
    return &slots_[static_cast<std::size_t>(slotIndex)];
}

const std::vector<EngineSlot>& EngineKit::slots() const noexcept
{
    return slots_;
}

} // namespace drumprog::engine
