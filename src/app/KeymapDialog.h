#pragma once

#include "ui/KeymapPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drumprog::app
{

/// Humble object (E-03): dialog "Tastatur-Mapping" (F-IN-02) with all slots, learning mode (click the
/// key column, press a key), duplicate warning and "Standard wiederherstellen". All logic is in
/// ui::KeymapPresenter.
class KeymapDialog final : public juce::Component, private juce::ListBoxModel, private juce::Timer
{
public:
    explicit KeymapDialog(ui::KeymapPresenter& presenter);
    ~KeymapDialog() override;

    KeymapDialog(const KeymapDialog&) = delete;
    KeymapDialog& operator=(const KeymapDialog&) = delete;
    KeymapDialog(KeymapDialog&&) = delete;
    KeymapDialog& operator=(KeymapDialog&&) = delete;

    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    int getNumRows() override;
    void
    paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;
    void timerCallback() override;
    void refresh();

    ui::KeymapPresenter& presenter_;
    juce::Label hint_;
    juce::ListBox list_{"Tastatur-Mapping", this};
    juce::Label warning_;
    juce::TextButton clearButton_{"Taste entfernen"};
    juce::TextButton defaultsButton_{"Standard wiederherstellen"};

    JUCE_LEAK_DETECTOR(KeymapDialog)
};

} // namespace drumprog::app
