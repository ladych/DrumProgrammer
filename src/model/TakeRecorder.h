#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <optional>
#include <string>

namespace drumprog::model
{

enum class RecordMode : std::uint8_t
{
    overdub, ///< each pass adds to the pattern (default)
    replace  ///< the pattern's notes are removed when the recording starts
};

/// Writes a recording run into a pattern of the project model (F-IN-07, F-IN-09): every hit becomes a
/// violet note (origin live), unquantised, and the whole run from start to stop is one undo step.
/// GUI thread only.
class TakeRecorder
{
public:
    static constexpr const char* kUndoName = "Aufnahme";

    TakeRecorder(juce::ValueTree project, juce::ValueTree globalKit, juce::UndoManager& undoManager);

    /// Starts a run into the pattern at patternIndex; false if there is no such pattern.
    bool begin(int patternIndex, RecordMode mode);
    /// Adds a note for the slot at slotIndex of the active kit, 1/16 long and cut at the pattern end, at the
    /// tick folded into the pattern. Ignored without a run, for unknown slots or a removed pattern.
    void add(int slotIndex, int velocity, std::int64_t tick);
    /// Ends the run; later changes are separate undo steps.
    void end();
    [[nodiscard]] bool isRecording() const noexcept { return patternId_.has_value(); }

private:
    juce::ValueTree project_;
    juce::ValueTree globalKit_;
    juce::UndoManager& undoManager_;
    std::optional<std::string> patternId_;
};

} // namespace drumprog::model
