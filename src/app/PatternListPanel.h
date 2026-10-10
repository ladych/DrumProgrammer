#pragma once

#include "ui/PatternListPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <functional>

namespace drumprog::app
{

/// What the context menu of the pattern list opens as dialogs.
struct PatternDialogs
{
    std::function<void(int index)> rename;
    std::function<void(int index)> setLength;
    /// Asks first if the song uses the pattern (F-SO-05).
    std::function<void(int index)> remove;
};

/// Humble object (E-03): pattern list (Pflichtenheft 6.1, area 4, F-SO-01) with colour dot, name and
/// length per row, "+" to add and a context menu for rename, colour, length, duplicate and delete. Rows
/// are dragged onto the song timeline.
/// Logic in ui::PatternListPresenter.
class PatternListPanel final : public juce::Component, private juce::ListBoxModel, private juce::Timer
{
public:
    PatternListPanel(ui::PatternListPresenter& presenter, PatternDialogs dialogs);
    ~PatternListPanel() override;

    PatternListPanel(const PatternListPanel&) = delete;
    PatternListPanel& operator=(const PatternListPanel&) = delete;
    PatternListPanel(PatternListPanel&&) = delete;
    PatternListPanel& operator=(PatternListPanel&&) = delete;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    int getNumRows() override;
    void
    paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& event) override;
    /// Rows can be dragged onto the song timeline (F-SO-02).
    juce::var getDragSourceDescription(const juce::SparseSet<int>& rowsToDescribe) override;
    void timerCallback() override;

    void refresh();
    void showMenu(int row);

    ui::PatternListPresenter& presenter_;
    PatternDialogs dialogs_;
    std::uint32_t seenChangeCount_ = 0;
    int seenSelection_ = -1;
    juce::TextButton addButton_{"+"};
    juce::ListBox list_{"Patterns", this};

    JUCE_LEAK_DETECTOR(PatternListPanel)
};

} // namespace drumprog::app
