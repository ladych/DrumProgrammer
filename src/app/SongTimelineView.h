#pragma once

#include "ui/SongTimelineGeometry.h"
#include "ui/SongTimelinePresenter.h"
#include "ui/TransportPresenter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <memory>
#include <optional>

namespace drumprog::app
{

/// Humble object (E-03): song timeline (Pflichtenheft 6.1, area 6) with bar ruler, the drums track with
/// coloured pattern blocks, ghost frames while dragging, the playback cursor, a scroll bar and zoom
/// (F-SO-02 to 07, F-PR-11). Patterns are dropped from the pattern list (drag description
/// kPatternDragPrefix + index). All logic is in ui::SongTimelinePresenter and ui::SongTimelineGeometry;
/// a 30 Hz timer repaints on changes.
class SongTimelineView final : public juce::Component, private juce::Timer, private juce::ScrollBar::Listener
{
public:
    static constexpr const char* kPatternDragPrefix = "pattern:";
    static constexpr int kPreferredHeight = 124;

    SongTimelineView(ui::SongTimelinePresenter& presenter, ui::TransportPresenter& transport);
    ~SongTimelineView() override;

    SongTimelineView(const SongTimelineView&) = delete;
    SongTimelineView& operator=(const SongTimelineView&) = delete;
    SongTimelineView(SongTimelineView&&) = delete;
    SongTimelineView& operator=(SongTimelineView&&) = delete;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class Track;

    void timerCallback() override;
    void scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    void contentChanged();
    void updateScrollBar();
    void zoom(bool in);
    void paintRuler(juce::Graphics& g) const;

    ui::SongTimelinePresenter& presenter_;
    ui::TransportPresenter& transport_;
    ui::SongTimelineGeometry geometry_;
    std::uint32_t seenChangeCount_ = 0;
    std::optional<std::int64_t> playhead_;

    juce::Rectangle<int> headerArea_;
    juce::Rectangle<int> rulerArea_;
    juce::Rectangle<int> trackLabelArea_;
    std::unique_ptr<Track> track_;
    juce::ScrollBar scrollBar_{false};
    juce::TextButton zoomOutButton_{"-"};
    juce::TextButton zoomInButton_{"+"};

    JUCE_LEAK_DETECTOR(SongTimelineView)
};

} // namespace drumprog::app
