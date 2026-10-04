#pragma once

#include "model/IIdGenerator.h"
#include "model/Project.h"
#include "model/TreeChangeListener.h"
#include "ui/ActivePattern.h"

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <string>
#include <vector>

namespace drumprog::ui
{

/// Logic of the pattern list (F-SO-01, humble view app/PatternListPanel) and the menu "Pattern": add,
/// rename, colour, duplicate, delete and length (F-PR-09). Clicking a row makes it the active pattern,
/// which the piano roll shows and the transport plays. Every change is one undo step (F-PJ-05). GUI
/// thread only.
class PatternListPresenter
{
public:
    static constexpr int kMinBars = 1;
    static constexpr int kMaxBars = 64;
    static constexpr int kNewPatternBars = 2;

    PatternListPresenter(juce::ValueTree project,
                         juce::UndoManager& undoManager,
                         model::IIdGenerator& idGenerator,
                         ActivePattern& active);

    /// Colours offered in the context menu, as "#RRGGBB"; new patterns take them in turn.
    [[nodiscard]] static std::vector<std::string> palette();

    [[nodiscard]] int numPatterns() const;
    // Per-row getters return empty values for indices outside the list.
    [[nodiscard]] std::string name(int index) const;
    [[nodiscard]] std::string colour(int index) const;
    [[nodiscard]] int lengthBars(int index) const;
    /// "1 Takt", "2 Takte".
    [[nodiscard]] std::string lengthText(int index) const;

    void select(int index) { active_.select(index); }
    [[nodiscard]] int selectedIndex() const noexcept { return active_.index(); }

    // Edits ignore indices outside the list.
    /// Appends "Pattern n" with two bars and makes it active.
    void add();
    /// Ignores names that are empty after trimming.
    void rename(int index, const std::string& name);
    void setColour(int index, const std::string& colour);
    /// Clamped to 1-64 bars. Notes behind the new end stay in the pattern but do not play.
    void setLengthBars(int index, int bars);
    /// Inserts "<name> Kopie" with the same notes after the pattern and makes it active.
    void duplicate(int index);
    /// The last pattern cannot be deleted, so there is always one to play.
    [[nodiscard]] bool canRemove() const;
    void remove(int index);

    /// Changes whenever the project changed, also by undo or loading.
    [[nodiscard]] std::uint32_t changeCount() const noexcept { return changeCount_; }

private:
    [[nodiscard]] model::Project project() const;
    [[nodiscard]] bool isValid(int index) const;
    [[nodiscard]] bool nameExists(const std::string& name) const;
    [[nodiscard]] std::string freeName() const;

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
    model::IIdGenerator& idGenerator_;
    ActivePattern& active_;
    std::uint32_t changeCount_ = 0;
    model::TreeChangeListener listener_;
};

} // namespace drumprog::ui
