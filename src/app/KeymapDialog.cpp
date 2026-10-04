#include "app/KeymapDialog.h"

#include "app/KeyboardInputGlue.h"

namespace drumprog::app
{
namespace
{

constexpr int kRowHeight = 24;
constexpr int kNoteWidth = 50;
constexpr int kNameWidth = 200;
constexpr int kKeyWidth = 130;
constexpr int kRefreshHz = 10;

juce::String utf8(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

} // namespace

KeymapDialog::KeymapDialog(ui::KeymapPresenter& presenter) : presenter_(presenter)
{
    hint_.setText(utf8("Klick in die Spalte „Taste“, dann die gewünschte Taste drücken. Esc bricht ab."),
                  juce::dontSendNotification);
    warning_.setColour(juce::Label::textColourId, juce::Colours::orangered);
    list_.setRowHeight(kRowHeight);
    clearButton_.onClick = [this]
    {
        presenter_.clearKey(list_.getSelectedRow());
        refresh();
    };
    defaultsButton_.onClick = [this]
    {
        presenter_.restoreDefaults();
        refresh();
    };

    for (auto* component :
         std::initializer_list<juce::Component*>{&hint_, &list_, &warning_, &clearButton_, &defaultsButton_})
        addAndMakeVisible(component);
    setWantsKeyboardFocus(true);
    setSize(kNoteWidth + kNameWidth + kKeyWidth + 60, 560);
    refresh();
    startTimerHz(kRefreshHz);
}

KeymapDialog::~KeymapDialog()
{
    stopTimer();
    presenter_.cancelLearning();
}

void KeymapDialog::resized()
{
    auto area = getLocalBounds().reduced(12);
    hint_.setBounds(area.removeFromTop(40));
    auto buttons = area.removeFromBottom(28);
    clearButton_.setBounds(buttons.removeFromLeft(150));
    defaultsButton_.setBounds(buttons.removeFromRight(200));
    area.removeFromBottom(8);
    warning_.setBounds(area.removeFromBottom(48));
    list_.setBounds(area);
}

bool KeymapDialog::keyPressed(const juce::KeyPress& key)
{
    if (!presenter_.learningRow())
        return false;
    if (const auto scancode = scancodeOf(key))
        presenter_.captureKey(*scancode);
    refresh();
    return true;
}

int KeymapDialog::getNumRows()
{
    return presenter_.numRows();
}

void KeymapDialog::paintListBoxItem(
    int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    const auto& laf = getLookAndFeel();
    if (rowIsSelected)
        g.fillAll(laf.findColour(juce::TextEditor::highlightColourId));

    g.setColour(laf.findColour(juce::Label::textColourId));
    g.drawText(
        utf8(presenter_.noteLabel(rowNumber)), 4, 0, kNoteWidth, height, juce::Justification::centredLeft);
    g.drawText(utf8(presenter_.slotName(rowNumber)),
               kNoteWidth,
               0,
               kNameWidth,
               height,
               juce::Justification::centredLeft);

    const auto keyArea = juce::Rectangle<int>(width - kKeyWidth, 2, kKeyWidth - 8, height - 4).toFloat();
    if (presenter_.learningRow() == rowNumber)
    {
        g.setColour(laf.findColour(juce::TextEditor::focusedOutlineColourId));
        g.fillRoundedRectangle(keyArea, 4.0F);
    }
    g.setColour(presenter_.isDuplicate(rowNumber) ? juce::Colours::orangered
                                                  : laf.findColour(juce::TextEditor::outlineColourId));
    g.drawRoundedRectangle(keyArea, 4.0F, presenter_.isDuplicate(rowNumber) ? 2.0F : 1.0F);
    g.setColour(laf.findColour(juce::Label::textColourId));
    g.drawText(utf8(presenter_.keyLabel(rowNumber)), keyArea, juce::Justification::centred);
}

void KeymapDialog::listBoxItemClicked(int row, const juce::MouseEvent& event)
{
    if (event.x >= list_.getVisibleRowWidth() - kKeyWidth)
    {
        presenter_.startLearning(row);
        grabKeyboardFocus();
    }
    refresh();
}

void KeymapDialog::timerCallback()
{
    refresh();
}

void KeymapDialog::refresh()
{
    warning_.setText(utf8(presenter_.duplicateWarning()), juce::dontSendNotification);
    list_.repaint();
}

} // namespace drumprog::app
