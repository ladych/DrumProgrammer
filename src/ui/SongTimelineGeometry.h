#pragma once

namespace drumprog::ui
{

/// Zoom and scroll of the song timeline (F-SO-07): converts between pixels of the visible track and
/// bars of the song. x is relative to the left edge of the track, whose width the view sets; the
/// content is the song plus room behind it to drop new blocks. GUI thread only.
class SongTimelineGeometry
{
public:
    static constexpr double kMinPixelsPerBar = 8.0;
    static constexpr double kMaxPixelsPerBar = 256.0;
    static constexpr double kDefaultPixelsPerBar = 48.0;
    static constexpr double kZoomFactor = 1.5;
    /// Empty bars behind the song.
    static constexpr int kExtraBars = 16;
    /// Bar numbers of the ruler are at least this far apart.
    static constexpr double kMinLabelPixels = 32.0;

    void setViewWidth(int width);
    /// Keeps the scroll position inside the content.
    void setSongLength(int bars);

    /// Bars may be fractional, e.g. for the playhead.
    [[nodiscard]] double xOfBar(double bar) const;
    /// Bar under x, never negative.
    [[nodiscard]] int barAt(double x) const;
    /// Bar under x with its fraction, also before the song start, e.g. for the waveform.
    [[nodiscard]] double exactBarAt(double x) const;
    [[nodiscard]] double pixelsPerBar() const noexcept { return pixelsPerBar_; }

    /// Both keep the bar at the left edge in place.
    void zoomIn();
    void zoomOut();
    [[nodiscard]] bool canZoomIn() const;
    [[nodiscard]] bool canZoomOut() const;
    void scrollBy(double dx);
    [[nodiscard]] double scrollX() const noexcept { return scrollX_; }

    [[nodiscard]] int firstVisibleBar() const;
    [[nodiscard]] int lastVisibleBar() const;
    /// Bars between two numbers of the ruler: 1, 2, 4, 8, ...
    [[nodiscard]] int labelStep() const;

private:
    void zoomTo(double pixelsPerBar);
    void clampScroll();

    int width_ = 0;
    int songBars_ = 0;
    double pixelsPerBar_ = kDefaultPixelsPerBar;
    double scrollX_ = 0.0;
};

} // namespace drumprog::ui
