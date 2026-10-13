#include "app/Splitter.h"

#include "app/Theme.h"

#include <utility>

namespace drumprog::app
{

Splitter::Splitter(Direction direction) : direction_(direction)
{
    setMouseCursor(direction == Direction::leftRight ? juce::MouseCursor::LeftRightResizeCursor
                                                     : juce::MouseCursor::UpDownResizeCursor);
    setWantsKeyboardFocus(false);
    setMouseClickGrabsKeyboardFocus(false);
}

void Splitter::setDragHandlers(std::function<void()> onDragStart, std::function<void(int offset)> onDrag)
{
    onDragStart_ = std::move(onDragStart);
    onDrag_ = std::move(onDrag);
}

void Splitter::paint(juce::Graphics& g)
{
    if (!active_)
        return;
    g.setColour(theme::colours().sliderFill);
    const auto area = getLocalBounds();
    if (direction_ == Direction::leftRight)
        g.fillRect(area.withSizeKeepingCentre(2, area.getHeight()));
    else
        g.fillRect(area.withSizeKeepingCentre(area.getWidth(), 2));
}

void Splitter::mouseEnter(const juce::MouseEvent& /*event*/)
{
    active_ = true;
    repaint();
}

void Splitter::mouseExit(const juce::MouseEvent& event)
{
    active_ = event.mods.isLeftButtonDown();
    repaint();
}

void Splitter::mouseDown(const juce::MouseEvent& /*event*/)
{
    if (onDragStart_)
        onDragStart_();
}

void Splitter::mouseDrag(const juce::MouseEvent& event)
{
    if (onDrag_)
        onDrag_(direction_ == Direction::leftRight ? event.getDistanceFromDragStartX()
                                                   : event.getDistanceFromDragStartY());
}

void Splitter::mouseUp(const juce::MouseEvent& event)
{
    active_ = isMouseOver() || contains(event.getPosition());
    repaint();
}

} // namespace drumprog::app
