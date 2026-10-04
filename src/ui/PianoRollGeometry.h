#pragma once

#include <cstdint>
#include <vector>

namespace drumprog::ui
{

/// Strength of a vertical line in the piano roll grid (F-PR-08): bar lines are drawn strongest.
enum class GridLineKind : std::uint8_t
{
    bar,
    beat,
    step
};

struct GridLine
{
    std::int64_t tick = 0;
    GridLineKind kind = GridLineKind::step;
};

/// Zoom and scroll of the piano roll and its velocity lane (F-PR-08): converts between pixels of the
/// visible note area and ticks and rows of the pattern. x and y are relative to the note area, whose
/// size the view sets; the content is the whole pattern with all rows. GUI thread only.
class PianoRollGeometry
{
public:
    static constexpr double kMinPixelsPerQuarter = 12.0;
    static constexpr double kMaxPixelsPerQuarter = 480.0;
    static constexpr double kDefaultPixelsPerQuarter = 96.0;
    static constexpr int kMinRowHeight = 12;
    static constexpr int kMaxRowHeight = 48;
    static constexpr int kDefaultRowHeight = 24;
    /// Grid steps closer than this are not drawn, only beats and bars.
    static constexpr double kMinStepPixels = 6.0;

    explicit PianoRollGeometry(int ticksPerQuarter);

    void setViewSize(int width, int height);
    /// Length of the pattern and number of rows; keeps the scroll position inside the content.
    void setContent(std::int64_t lengthTicks, int numRows);

    [[nodiscard]] double xOf(std::int64_t tick) const;
    [[nodiscard]] std::int64_t tickAt(double x) const;
    /// Ticks covered by a distance in pixels, e.g. the grab zone at a note's end.
    [[nodiscard]] std::int64_t ticksFor(double pixels) const;
    [[nodiscard]] double yOf(int row) const;
    /// Row under y; may be outside 0..numRows-1.
    [[nodiscard]] int rowAt(double y) const;
    [[nodiscard]] int rowHeight() const noexcept { return rowHeight_; }
    [[nodiscard]] double pixelsPerQuarter() const noexcept { return pixelsPerQuarter_; }

    /// Zooms around the anchor, which stays at the same tick or row.
    void zoomHorizontally(double factor, double anchorX);
    void zoomVertically(double factor, double anchorY);
    void scrollBy(double dx, double dy);
    [[nodiscard]] double scrollX() const noexcept { return scrollX_; }
    [[nodiscard]] double scrollY() const noexcept { return scrollY_; }

    [[nodiscard]] std::int64_t firstVisibleTick() const;
    [[nodiscard]] std::int64_t lastVisibleTick() const;
    [[nodiscard]] int firstVisibleRow() const;
    [[nodiscard]] int lastVisibleRow() const;

    /// Vertical lines of the visible part of the pattern; steps only where they are far enough apart.
    [[nodiscard]] std::vector<GridLine>
    gridLines(std::int64_t barTicks, std::int64_t beatTicks, std::int64_t stepTicks) const;

private:
    [[nodiscard]] double pixelsPerTick() const;
    void clampScroll();

    int ticksPerQuarter_;
    int width_ = 0;
    int height_ = 0;
    std::int64_t lengthTicks_ = 0;
    int numRows_ = 0;
    double pixelsPerQuarter_ = kDefaultPixelsPerQuarter;
    int rowHeight_ = kDefaultRowHeight;
    double scrollX_ = 0.0;
    double scrollY_ = 0.0;
};

} // namespace drumprog::ui
