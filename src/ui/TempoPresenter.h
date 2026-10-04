#pragma once

#include "model/Project.h"

#include <juce_data_structures/juce_data_structures.h>

#include <string>

namespace drumprog::ui
{

/// BPM and time signature fields of the transport bar (F-TR-03). Every change edits the project
/// model and is one undo step; the sequencer follows through the snapshot. GUI thread only.
class TempoPresenter
{
public:
    static constexpr double kMinBpm = 30.0;
    static constexpr double kMaxBpm = 300.0;
    static constexpr int kMaxNumerator = 16;

    TempoPresenter(juce::ValueTree project, juce::UndoManager& undoManager);

    [[nodiscard]] double bpm() const;
    /// Two decimals, e.g. "120.00".
    [[nodiscard]] std::string bpmText() const;
    /// Clamped to 30..300 and rounded to two decimals.
    void setBpm(double bpm);
    /// Accepts "97.5" and "97,5"; false and no change if the text is no number.
    bool setBpmText(const std::string& text);

    [[nodiscard]] model::TimeSignature timeSignature() const;
    /// E.g. "6/8".
    [[nodiscard]] std::string timeSignatureText() const;
    /// Numerator clamped to 1..16; ignored unless the denominator is 4, 8 or 16.
    void setTimeSignature(int numerator, int denominator);
    [[nodiscard]] static bool isValidDenominator(int denominator) noexcept;

private:
    [[nodiscard]] model::Project project() const;

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
};

} // namespace drumprog::ui
