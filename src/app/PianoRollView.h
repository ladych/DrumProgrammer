#pragma once

#include "ui/PianoRollGeometry.h"
#include "ui/PianoRollPresenter.h"
#include "ui/TransportPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <optional>

namespace drumprog::app
{

/// Humble object (E-03): piano roll (Pflichtenheft 6.1, area 8) with header, row names and key
/// badges, the note area with grid, notes, ghost frames and playback cursor (F-PR-01 to 08, F-PR-11),
/// scroll bars and zoom, and the velocity lane below (area 9, F-PR-10). All logic is in
/// ui::PianoRollPresenter and ui::PianoRollGeometry; a 30 Hz timer repaints on changes.
class PianoRollView final : public juce::Component, private juce::Timer, private juce::ScrollBar::Listener
{
public:
    PianoRollView(ui::PianoRollPresenter& presenter, ui::TransportPresenter& transport);
    ~PianoRollView() override;

    PianoRollView(const PianoRollView&) = delete;
    PianoRollView& operator=(const PianoRollView&) = delete;
    PianoRollView(PianoRollView&&) = delete;
    PianoRollView& operator=(PianoRollView&&) = delete;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class NoteArea;
    class VelocityLane;

    void timerCallback() override;
    void scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    void contentChanged();
    void updateScrollBars();
    void zoom(double factor);
    void paintRows(juce::Graphics& g) const;
    void paintLegend(juce::Graphics& g, juce::Rectangle<int> area) const;

    ui::PianoRollPresenter& presenter_;
    ui::TransportPresenter& transport_;
    ui::PianoRollGeometry geometry_;
    std::uint32_t seenChangeCount_ = 0;
    std::optional<std::int64_t> playhead_;

    juce::Rectangle<int> headerArea_;
    juce::Rectangle<int> rowsArea_;
    juce::Rectangle<int> laneHeaderArea_;
    std::unique_ptr<NoteArea> noteArea_;
    std::unique_ptr<VelocityLane> velocityLane_;
    juce::ScrollBar horizontalBar_{false};
    juce::ScrollBar verticalBar_{true};
    juce::TextButton zoomOutButton_{"-"};
    juce::TextButton zoomInButton_{"+"};

    JUCE_LEAK_DETECTOR(PianoRollView)
};

} // namespace drumprog::app
