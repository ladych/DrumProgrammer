#include "app/PianoRollToolbar.h"

#include "app/Theme.h"
#include "i18n/Translate.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 10;
constexpr int kToolGroup = 1;
constexpr int kRecordModeGroup = 2;
constexpr int kButtonGap = 2;

} // namespace

PianoRollToolbar::PianoRollToolbar(ui::PianoRollPresenter& presenter, ui::TransportPresenter& transport)
    : presenter_(presenter), transport_(transport)
{
    const std::array<std::pair<IconButton*, ui::PianoRollTool>, 3> tools{
        {{&drawButton_, ui::PianoRollTool::draw},
         {&selectButton_, ui::PianoRollTool::select},
         {&eraseButton_, ui::PianoRollTool::erase}}};
    for (const auto& [button, tool] : tools)
    {
        button->setClickingTogglesState(true);
        button->setRadioGroupId(kToolGroup);
        button->onClick = [this, tool] { presenter_.setTool(tool); };
    }
    const std::array<std::pair<IconButton*, model::RecordMode>, 2> modes{
        {{&overdubButton_, model::RecordMode::overdub}, {&replaceButton_, model::RecordMode::replace}}};
    for (const auto& [button, mode] : modes)
    {
        button->setClickingTogglesState(true);
        button->setRadioGroupId(kRecordModeGroup);
        button->onClick = [this, mode] { transport_.setRecordMode(mode); };
    }
    for (auto* button : {&drawButton_, &selectButton_, &eraseButton_, &overdubButton_, &replaceButton_})
        addAndMakeVisible(button);
    refresh();
    startTimerHz(kTimerHz);
}

PianoRollToolbar::~PianoRollToolbar()
{
    stopTimer();
}

void PianoRollToolbar::resized()
{
    auto area = getLocalBounds();
    for (auto* button : {&drawButton_, &selectButton_, &eraseButton_})
    {
        button->setBounds(area.removeFromLeft(kButtonSize));
        area.removeFromLeft(kButtonGap);
    }
    area.removeFromLeft(kGroupGap - kButtonGap);
    overdubButton_.setBounds(area.removeFromLeft(kButtonSize));
    area.removeFromLeft(kButtonGap);
    replaceButton_.setBounds(area.removeFromLeft(kButtonSize));
}

void PianoRollToolbar::timerCallback()
{
    refresh();
}

void PianoRollToolbar::refresh()
{
    // Strg+1/2/3 and the menu change the presenter directly, so the buttons follow it.
    drawButton_.setToggleState(presenter_.tool() == ui::PianoRollTool::draw, juce::dontSendNotification);
    selectButton_.setToggleState(presenter_.tool() == ui::PianoRollTool::select, juce::dontSendNotification);
    eraseButton_.setToggleState(presenter_.tool() == ui::PianoRollTool::erase, juce::dontSendNotification);
    const bool replace = transport_.recordMode() == model::RecordMode::replace;
    overdubButton_.setToggleState(!replace, juce::dontSendNotification);
    replaceButton_.setToggleState(replace, juce::dontSendNotification);
}

SnapControls::SnapControls(ui::PianoRollPresenter& presenter) : presenter_(presenter)
{
    for (std::size_t index = 0; index < ui::kGridDivisions.size(); ++index)
        gridBox_.addItem(ui::gridLabel(ui::kGridDivisions.at(index)), static_cast<int>(index) + 1);
    gridBox_.onChange = [this]
    {
        const int index = gridBox_.getSelectedItemIndex();
        if (index >= 0)
            presenter_.setGrid(ui::kGridDivisions.at(static_cast<std::size_t>(index)));
    };
    gridBox_.setTooltip(i18n::trJuce("Grid"));
    snapButton_.setTooltip(i18n::trJuce("Snap notes to the grid (free while holding Alt)"));
    snapButton_.onClick = [this] { presenter_.setSnapEnabled(snapButton_.getToggleState()); };
    for (auto* component : std::initializer_list<juce::Component*>{&snapButton_, &gridBox_, &altHint_})
    {
        component->setWantsKeyboardFocus(false);
        component->setMouseClickGrabsKeyboardFocus(false);
        addAndMakeVisible(component);
    }
    refresh();
    startTimerHz(kTimerHz);
}

SnapControls::~SnapControls()
{
    stopTimer();
}

void SnapControls::resized()
{
    auto area = getLocalBounds();
    snapButton_.setBounds(area.removeFromLeft(64));
    gridBox_.setBounds(area.removeFromLeft(76));
    altHint_.setBounds(area.removeFromLeft(80));
}

void SnapControls::timerCallback()
{
    refresh();
}

void SnapControls::refresh()
{
    snapButton_.setToggleState(presenter_.snapEnabled(), juce::dontSendNotification);
    const auto index =
        std::distance(ui::kGridDivisions.begin(), std::ranges::find(ui::kGridDivisions, presenter_.grid()));
    gridBox_.setSelectedItemIndex(static_cast<int>(index), juce::dontSendNotification);
}

} // namespace drumprog::app
