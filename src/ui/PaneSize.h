#pragma once

namespace drumprog::ui
{

/// Size in pixels of a pane that the user changes with a splitter (AP14), e.g. the pattern list column or
/// the velocity lane. The size the user dragged to is kept; a small window only limits what is shown, so
/// the pane comes back to that size when the window grows again.
class PaneSize
{
public:
    PaneSize(int preferred, int minimum) noexcept;

    /// Shown size: the preferred size, at most what leaves the other panes othersMinimum of the available
    /// space, but never less than the own minimum.
    [[nodiscard]] int size(int available, int othersMinimum) const noexcept;

    /// Mouse down on the splitter: the drag starts from the shown size.
    void beginDrag(int available, int othersMinimum) noexcept;

    /// offset: distance of the mouse from where the drag started, positive = the pane grows.
    void dragTo(int offset, int available, int othersMinimum) noexcept;

    [[nodiscard]] int preferred() const noexcept { return preferred_; }

private:
    int preferred_;
    int minimum_;
    int dragStart_;
};

} // namespace drumprog::ui
