#include "app/PianoRollToolbar.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 10;
constexpr int kToolGroup = 1;

} // namespace

PianoRollToolbar::PianoRollToolbar(ui::PianoRollPresenter& presenter) : presenter_(presenter)
{
    setUpGrid();
    setUpTools();
    for (auto* component : std::initializer_list<juce::Component*>{
             &snapButton_, &gridBox_, &altHint_, &drawButton_, &selectButton_, &eraseButton_})
    {
        component->setWantsKeyboardFocus(false);
        addAndMakeVisible(component);
    }
    refresh();
    startTimerHz(kTimerHz);
}

void PianoRollToolbar::setUpGrid()
{
    for (std::size_t index = 0; index < ui::kGridDivisions.size(); ++index)
        gridBox_.addItem(ui::gridLabel(ui::kGridDivisions.at(index)), static_cast<int>(index) + 1);
    gridBox_.onChange = [this]
    {
        const int index = gridBox_.getSelectedItemIndex();
        if (index >= 0)
            presenter_.setGrid(ui::kGridDivisions.at(static_cast<std::size_t>(index)));
    };
    snapButton_.onClick = [this] { presenter_.setSnapEnabled(snapButton_.getToggleState()); };
}

void PianoRollToolbar::setUpTools()
{

    const std::array<std::pair<juce::TextButton*, ui::PianoRollTool>, 3> tools{
        {{&drawButton_, ui::PianoRollTool::draw},
         {&selectButton_, ui::PianoRollTool::select},
         {&eraseButton_, ui::PianoRollTool::erase}}};
    for (const auto& [button, tool] : tools)
    {
        button->setClickingTogglesState(true);
        button->setRadioGroupId(kToolGroup);
        button->onClick = [this, tool] { presenter_.setTool(tool); };
    }
    drawButton_.setTooltip("Strg+1");
    selectButton_.setTooltip("Strg+2");
    eraseButton_.setTooltip("Strg+3");
}

PianoRollToolbar::~PianoRollToolbar()
{
    stopTimer();
}

void PianoRollToolbar::resized()
{
    auto area = getLocalBounds();
    snapButton_.setBounds(area.removeFromLeft(70));
    gridBox_.setBounds(area.removeFromLeft(80).reduced(0, 2));
    altHint_.setBounds(area.removeFromLeft(80));
    area.removeFromLeft(8);
    drawButton_.setBounds(area.removeFromLeft(90).reduced(2, 0));
    selectButton_.setBounds(area.removeFromLeft(90).reduced(2, 0));
    eraseButton_.setBounds(area.removeFromLeft(90).reduced(2, 0));
}

void PianoRollToolbar::timerCallback()
{
    refresh();
}

void PianoRollToolbar::refresh()
{
    // Strg+1/2/3 and the menu change the presenter directly, so the buttons follow it.
    snapButton_.setToggleState(presenter_.snapEnabled(), juce::dontSendNotification);
    const auto* const grid =
        std::find(ui::kGridDivisions.begin(), ui::kGridDivisions.end(), presenter_.grid());
    gridBox_.setSelectedItemIndex(static_cast<int>(grid - ui::kGridDivisions.begin()),
                                  juce::dontSendNotification);
    drawButton_.setToggleState(presenter_.tool() == ui::PianoRollTool::draw, juce::dontSendNotification);
    selectButton_.setToggleState(presenter_.tool() == ui::PianoRollTool::select, juce::dontSendNotification);
    eraseButton_.setToggleState(presenter_.tool() == ui::PianoRollTool::erase, juce::dontSendNotification);
}

} // namespace drumprog::app
