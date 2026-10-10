#include "ui/SongTimelineGeometry.h"

#include <algorithm>
#include <cmath>

namespace drumprog::ui
{

void SongTimelineGeometry::setViewWidth(int width)
{
    width_ = std::max(width, 0);
    clampScroll();
}

void SongTimelineGeometry::setSongLength(int bars)
{
    songBars_ = std::max(bars, 0);
    clampScroll();
}

double SongTimelineGeometry::xOfBar(double bar) const
{
    return bar * pixelsPerBar_ - scrollX_;
}

int SongTimelineGeometry::barAt(double x) const
{
    return std::max(0, static_cast<int>(std::floor((x + scrollX_) / pixelsPerBar_)));
}

double SongTimelineGeometry::exactBarAt(double x) const
{
    return (x + scrollX_) / pixelsPerBar_;
}

void SongTimelineGeometry::zoomIn()
{
    zoomTo(pixelsPerBar_ * kZoomFactor);
}

void SongTimelineGeometry::zoomOut()
{
    zoomTo(pixelsPerBar_ / kZoomFactor);
}

bool SongTimelineGeometry::canZoomIn() const
{
    return pixelsPerBar_ < kMaxPixelsPerBar;
}

bool SongTimelineGeometry::canZoomOut() const
{
    return pixelsPerBar_ > kMinPixelsPerBar;
}

void SongTimelineGeometry::scrollBy(double dx)
{
    scrollX_ += dx;
    clampScroll();
}

int SongTimelineGeometry::firstVisibleBar() const
{
    return barAt(0.0);
}

int SongTimelineGeometry::lastVisibleBar() const
{
    return barAt(static_cast<double>(width_));
}

int SongTimelineGeometry::labelStep() const
{
    int step = 1;
    while (static_cast<double>(step) * pixelsPerBar_ < kMinLabelPixels)
        step *= 2;
    return step;
}

void SongTimelineGeometry::zoomTo(double pixelsPerBar)
{
    const double leftBar = scrollX_ / pixelsPerBar_;
    pixelsPerBar_ = std::clamp(pixelsPerBar, kMinPixelsPerBar, kMaxPixelsPerBar);
    scrollX_ = leftBar * pixelsPerBar_;
    clampScroll();
}

void SongTimelineGeometry::clampScroll()
{
    const double content = static_cast<double>(songBars_ + kExtraBars) * pixelsPerBar_;
    scrollX_ = std::clamp(scrollX_, 0.0, std::max(0.0, content - static_cast<double>(width_)));
}

} // namespace drumprog::ui
