#pragma once

#include "engine/SampleBuffer.h"

#include <array>
#include <memory>
#include <vector>

namespace drumprog::engine
{

/// One slot as the audio thread plays it: sample already at device rate,
/// gain and pitch already converted.
struct EngineSlot
{
    int slotIndex = 0;
    int midiNote = -1;                          ///< -1: not playable
    std::shared_ptr<const SampleBuffer> sample; ///< null: no sample, trigger only lights the LED
    float gain = 1.0F;
    double playbackRate = 1.0;
    int chokeGroup = 0;
};

/// Immutable kit snapshot handed to the audio thread. Lookup by MIDI note is a
/// table access, so it is safe in the audio callback (Q-04).
class EngineKit
{
public:
    static constexpr int kNumMidiNotes = 128;

    /// If several slots use the same MIDI note, the first one wins.
    explicit EngineKit(std::vector<EngineSlot> slots);

    /// Returns null for notes outside 0..127 or without slot.
    [[nodiscard]] const EngineSlot* slotForNote(int midiNote) const noexcept;
    /// Returns null for indices outside the kit.
    [[nodiscard]] const EngineSlot* slotAt(int slotIndex) const noexcept;
    [[nodiscard]] const std::vector<EngineSlot>& slots() const noexcept;

private:
    std::vector<EngineSlot> slots_;
    std::array<int, kNumMidiNotes> noteToSlot_{};
};

} // namespace drumprog::engine
