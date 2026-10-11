#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <functional>

namespace drumprog::app
{

/// Humble object (AP14): a bar between two areas that the user drags to change their sizes, like the
/// border between the overview and the chat in Claude Desktop. It lights up under the mouse and only
/// reports the drag; the owner keeps the size in a ui::PaneSize and lays out again.
class Splitter final : public juce::Component
{
public:
    enum class Direction : std::uint8_t
    {
        leftRight, ///< vertical bar between columns
        upDown,    ///< horizontal bar between rows
    };

    static constexpr int kThickness = 6;

    explicit Splitter(Direction direction);

    /// onDragStart: mouse down on the bar. onDrag: distance of the mouse from where the drag started,
    /// positive = to the right or down.
    void setDragHandlers(std::function<void()> onDragStart, std::function<void(int offset)> onDrag);

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    Direction direction_;
    std::function<void()> onDragStart_;
    std::function<void(int offset)> onDrag_;
    bool active_ = false;

    JUCE_LEAK_DETECTOR(Splitter)
};

} // namespace drumprog::app
