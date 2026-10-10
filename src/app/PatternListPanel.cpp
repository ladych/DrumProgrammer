#include "app/PatternListPanel.h"

#include "app/SongTimelineView.h"

#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 30;
constexpr int kTitleHeight = 28;
constexpr int kRowHeight = 26;

juce::String utf8(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

juce::Colour colourOf(const std::string& colour)
{
    return juce::Colour::fromString("FF" + utf8(colour).trimCharactersAtStart("#"));
}

} // namespace

PatternListPanel::PatternListPanel(ui::PatternListPresenter& presenter, PatternDialogs dialogs)
    : presenter_(presenter), dialogs_(std::move(dialogs))
{
    list_.setRowHeight(kRowHeight);
    list_.setWantsKeyboardFocus(false);
    addButton_.setWantsKeyboardFocus(false);
    addButton_.setTooltip("Neues Pattern");
    addButton_.onClick = [this] { presenter_.add(); };
    addAndMakeVisible(addButton_);
    addAndMakeVisible(list_);
    refresh();
    startTimerHz(kTimerHz);
}

PatternListPanel::~PatternListPanel()
{
    stopTimer();
}

void PatternListPanel::paint(juce::Graphics& g)
{
    g.setColour(findColour(juce::Label::textColourId));
    g.setFont(juce::FontOptions(13.0F, juce::Font::bold));
    g.drawText("PATTERNS",
               getLocalBounds().removeFromTop(kTitleHeight).reduced(6, 0),
               juce::Justification::centredLeft);
}

void PatternListPanel::resized()
{
    auto area = getLocalBounds();
    auto title = area.removeFromTop(kTitleHeight);
    addButton_.setBounds(title.removeFromRight(kTitleHeight).reduced(3));
    list_.setBounds(area);
}

int PatternListPanel::getNumRows()
{
    return presenter_.numPatterns();
}

void PatternListPanel::paintListBoxItem(
    int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= getNumRows())
        return;
    if (rowIsSelected)
        g.fillAll(findColour(juce::TextEditor::highlightColourId));
    auto area = juce::Rectangle<int>(0, 0, width, height).reduced(8, 0);
    g.setColour(colourOf(presenter_.colour(rowNumber)));
    g.fillRect(area.removeFromLeft(12).withSizeKeepingCentre(10, 10));
    area.removeFromLeft(8);
    g.setColour(findColour(juce::Label::textColourId).withAlpha(0.6F));
    g.drawText(
        utf8(presenter_.lengthText(rowNumber)), area.removeFromRight(70), juce::Justification::centredRight);
    g.setColour(findColour(juce::Label::textColourId));
    g.drawText(utf8(presenter_.name(rowNumber)), area, juce::Justification::centredLeft, true);
}

void PatternListPanel::listBoxItemClicked(int row, const juce::MouseEvent& event)
{
    presenter_.select(row);
    if (event.mods.isPopupMenu())
        showMenu(row);
}

void PatternListPanel::listBoxItemDoubleClicked(int row, const juce::MouseEvent& /*event*/)
{
    dialogs_.rename(row);
}

juce::var PatternListPanel::getDragSourceDescription(const juce::SparseSet<int>& rowsToDescribe)
{
    if (rowsToDescribe.isEmpty())
        return {};
    return juce::String(SongTimelineView::kPatternDragPrefix) + juce::String(rowsToDescribe[0]);
}

void PatternListPanel::timerCallback()
{
    if (presenter_.changeCount() != seenChangeCount_ || presenter_.selectedIndex() != seenSelection_)
        refresh();
}

void PatternListPanel::refresh()
{
    seenChangeCount_ = presenter_.changeCount();
    seenSelection_ = presenter_.selectedIndex();
    list_.updateContent();
    list_.selectRow(seenSelection_, true, true);
    list_.repaint();
}

void PatternListPanel::showMenu(int row)
{
    juce::PopupMenu colours;
    const auto palette = ui::PatternListPresenter::palette();
    for (std::size_t index = 0; index < palette.size(); ++index)
        colours.addColouredItem(
            static_cast<int>(index) + 100, utf8(palette[index]), colourOf(palette[index]));

    juce::PopupMenu menu;
    menu.addItem(1, "Umbenennen...");
    menu.addSubMenu("Farbe", colours);
    menu.addItem(2, juce::String::fromUTF8("L\xc3\xa4nge..."));
    menu.addItem(3, "Duplizieren");
    menu.addItem(4, juce::String::fromUTF8("L\xc3\xb6schen"), presenter_.canRemove());
    menu.showMenuAsync(juce::PopupMenu::Options{},
                       [this, row, palette](int result)
                       {
                           if (result == 1)
                               dialogs_.rename(row);
                           else if (result == 2)
                               dialogs_.setLength(row);
                           else if (result == 3)
                               presenter_.duplicate(row);
                           else if (result == 4)
                               dialogs_.remove(row);
                           else if (result >= 100)
                               presenter_.setColour(row, palette.at(static_cast<std::size_t>(result - 100)));
                       });
}

} // namespace drumprog::app
