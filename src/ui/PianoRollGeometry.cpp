#include "ui/PianoRollGeometry.h"

#include "engine/TempoMath.h"

#include <algorithm>
#include <cmath>

namespace drumprog::ui
{
namespace
{

GridLineKind kindOf(std::int64_t tick, std::int64_t barTicks, std::int64_t beatTicks)
{
    if (tick % barTicks == 0)
        return GridLineKind::bar;
    if (tick % beatTicks == 0)
        return GridLineKind::beat;
    return GridLineKind::step;
}

} // namespace

PianoRollGeometry::PianoRollGeometry(int ticksPerQuarter) : ticksPerQuarter_(ticksPerQuarter) {}

void PianoRollGeometry::setViewSize(int width, int height)
{
    width_ = std::max(width, 0);
    height_ = std::max(height, 0);
    clampScroll();
}

void PianoRollGeometry::setContent(std::int64_t lengthTicks, int numRows)
{
    lengthTicks_ = lengthTicks;
    numRows_ = numRows;
    clampScroll();
}

double PianoRollGeometry::xOf(std::int64_t tick) const
{
    return static_cast<double>(tick) * pixelsPerQuarter_ / ticksPerQuarter_ - scrollX_;
}

std::int64_t PianoRollGeometry::tickAt(double x) const
{
    return static_cast<std::int64_t>(std::floor((x + scrollX_) * ticksPerQuarter_ / pixelsPerQuarter_));
}

std::int64_t PianoRollGeometry::ticksFor(double pixels) const
{
    return static_cast<std::int64_t>(std::lround(pixels * ticksPerQuarter_ / pixelsPerQuarter_));
}

double PianoRollGeometry::yOf(int row) const
{
    return static_cast<double>(row) * rowHeight_ - scrollY_;
}

int PianoRollGeometry::rowAt(double y) const
{
    return static_cast<int>(std::floor((y + scrollY_) / rowHeight_));
}

void PianoRollGeometry::zoomHorizontally(double factor, double anchorX)
{
    const double anchorQuarters = (anchorX + scrollX_) / pixelsPerQuarter_;
    pixelsPerQuarter_ = std::clamp(pixelsPerQuarter_ * factor, kMinPixelsPerQuarter, kMaxPixelsPerQuarter);
    scrollX_ = anchorQuarters * pixelsPerQuarter_ - anchorX;
    clampScroll();
}

void PianoRollGeometry::zoomVertically(double factor, double anchorY)
{
    const double anchorRow = (anchorY + scrollY_) / rowHeight_;
    rowHeight_ = std::clamp(static_cast<int>(std::lround(rowHeight_ * factor)), kMinRowHeight, kMaxRowHeight);
    scrollY_ = anchorRow * rowHeight_ - anchorY;
    clampScroll();
}

void PianoRollGeometry::scrollBy(double dx, double dy)
{
    scrollX_ += dx;
    scrollY_ += dy;
    clampScroll();
}

std::int64_t PianoRollGeometry::firstVisibleTick() const
{
    return tickAt(0.0);
}

std::int64_t PianoRollGeometry::lastVisibleTick() const
{
    return tickAt(width_);
}

int PianoRollGeometry::firstVisibleRow() const
{
    return std::max(rowAt(0.0), 0);
}

int PianoRollGeometry::lastVisibleRow() const
{
    return std::min(rowAt(height_), numRows_ - 1);
}

std::vector<GridLine>
PianoRollGeometry::gridLines(std::int64_t barTicks, std::int64_t beatTicks, std::int64_t stepTicks) const
{
    const std::int64_t first = std::max<std::int64_t>(firstVisibleTick(), 0);
    const std::int64_t last = std::min(lastVisibleTick(), lengthTicks_);
    std::vector<GridLine> lines;
    const auto addEvery = [&](std::int64_t spacing)
    {
        for (std::int64_t tick = engine::floorDiv(first + spacing - 1, spacing) * spacing; tick <= last;
             tick += spacing)
        {
            lines.push_back({tick, kindOf(tick, barTicks, beatTicks)});
        }
    };
    // Triplet steps do not always meet the beats, so beats are added on their own.
    addEvery(beatTicks);
    if (static_cast<double>(stepTicks) * pixelsPerTick() >= kMinStepPixels)
        addEvery(stepTicks);
    std::ranges::sort(lines, {}, &GridLine::tick);
    const auto duplicates = std::ranges::unique(lines, {}, &GridLine::tick);
    lines.erase(duplicates.begin(), duplicates.end());
    return lines;
}

double PianoRollGeometry::pixelsPerTick() const
{
    return pixelsPerQuarter_ / ticksPerQuarter_;
}

void PianoRollGeometry::clampScroll()
{
    const double maxX = std::max(static_cast<double>(lengthTicks_) * pixelsPerTick() - width_, 0.0);
    const double maxY = std::max(static_cast<double>(numRows_) * rowHeight_ - height_, 0.0);
    scrollX_ = std::clamp(scrollX_, 0.0, maxX);
    scrollY_ = std::clamp(scrollY_, 0.0, maxY);
}

} // namespace drumprog::ui
