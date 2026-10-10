#pragma once

#include "ui/BackingTrackPresenter.h"
#include "ui/SongTimelineGeometry.h"
#include "ui/SongTimelinePresenter.h"
#include "ui/TransportPresenter.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace drumprog::app
{

/// Humble object (E-03): song timeline (Pflichtenheft 6.1, areas 6 and 7) with bar ruler, the backing track
/// lane with its waveform in the bar grid (F-BT-01, 04, 05), the drums track with coloured pattern blocks,
/// ghost frames while dragging, the playback cursor, a scroll bar and zoom (F-SO-02 to 07, F-PR-11). A click
/// on the ruler moves the song position there (F-BT-07); dragging the waveform moves the backing track.
/// Patterns are dropped from the pattern list (drag description kPatternDragPrefix + index). All logic is
/// in ui::SongTimelinePresenter, ui::BackingTrackPresenter and ui::SongTimelineGeometry; a 30 Hz timer
/// repaints on changes.
class SongTimelineView final : public juce::Component, private juce::Timer, private juce::ScrollBar::Listener
{
public:
    static constexpr const char* kPatternDragPrefix = "pattern:";
    static constexpr int kPreferredHeight = 176;

    /// loadBackingTrack opens the file dialog of the menu entry "Backing-Track laden".
    SongTimelineView(ui::SongTimelinePresenter& presenter,
                     ui::TransportPresenter& transport,
                     ui::BackingTrackPresenter& backing,
                     juce::AudioFormatManager& formats,
                     std::function<void()> loadBackingTrack);
    ~SongTimelineView() override;

    SongTimelineView(const SongTimelineView&) = delete;
    SongTimelineView& operator=(const SongTimelineView&) = delete;
    SongTimelineView(SongTimelineView&&) = delete;
    SongTimelineView& operator=(SongTimelineView&&) = delete;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    class Track;
    class BackingLane;

    void timerCallback() override;
    void scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    void contentChanged();
    void updateScrollBar();
    void zoom(bool in);
    void paintRuler(juce::Graphics& g) const;
    void paintTrackLabels(juce::Graphics& g) const;
    void paintPlayhead(juce::Graphics& g, int height) const;
    [[nodiscard]] int contentBars() const;

    ui::SongTimelinePresenter& presenter_;
    ui::TransportPresenter& transport_;
    ui::BackingTrackPresenter& backing_;
    std::function<void()> loadBackingTrack_;
    ui::SongTimelineGeometry geometry_;
    std::uint32_t seenChangeCount_ = 0;
    std::uint32_t seenBackingChangeCount_ = 0;
    std::optional<std::int64_t> playhead_;

    juce::Rectangle<int> headerArea_;
    juce::Rectangle<int> rulerArea_;
    juce::Rectangle<int> backingLabelArea_;
    juce::Rectangle<int> trackLabelArea_;
    std::unique_ptr<BackingLane> backingLane_;
    std::unique_ptr<Track> track_;
    juce::ScrollBar scrollBar_{false};
    juce::TextButton zoomOutButton_{"-"};
    juce::TextButton zoomInButton_{"+"};

    JUCE_LEAK_DETECTOR(SongTimelineView)
};

} // namespace drumprog::app
