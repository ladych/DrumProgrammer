#include "ui/PaneSize.h"

#include <algorithm>

namespace drumprog::ui
{

PaneSize::PaneSize(int preferred, int minimum) noexcept
    : preferred_(preferred), minimum_(minimum), dragStart_(preferred)
{
}

int PaneSize::size(int available, int othersMinimum) const noexcept
{
    return std::max(minimum_, std::min(preferred_, available - othersMinimum));
}

void PaneSize::beginDrag(int available, int othersMinimum) noexcept
{
    dragStart_ = size(available, othersMinimum);
}

void PaneSize::dragTo(int offset, int available, int othersMinimum) noexcept
{
    const int maximum = std::max(minimum_, available - othersMinimum);
    preferred_ = std::clamp(dragStart_ + offset, minimum_, maximum);
}

} // namespace drumprog::ui
