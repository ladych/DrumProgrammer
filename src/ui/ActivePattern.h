#pragma once

#include "model/TreeChangeListener.h"

#include <juce_data_structures/juce_data_structures.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace drumprog::ui
{

/// The pattern chosen in the pattern list (F-SO-01): it is shown in the piano roll and played by the
/// transport. It is kept by id, so it stays the same pattern when others are added, removed or
/// reordered; if it disappears (deleted, undone, another project loaded), the pattern now at its
/// place becomes active. GUI thread only.
class ActivePattern
{
public:
    explicit ActivePattern(juce::ValueTree project);

    /// Ignores indices outside the pattern list.
    void select(int index);
    /// -1 only while the project has no pattern.
    [[nodiscard]] int index() const noexcept { return index_; }
    /// The function is called with the new index whenever it changes.
    void addOnChange(std::function<void(int)> onChange) { onChange_.push_back(std::move(onChange)); }

private:
    void update();
    [[nodiscard]] int numPatterns() const;
    [[nodiscard]] int indexOf(const std::string& id) const;
    void setIndex(int index);

    juce::ValueTree project_;
    std::string id_;
    int index_ = -1;
    std::vector<std::function<void(int)>> onChange_;
    model::TreeChangeListener listener_;
};

} // namespace drumprog::ui
