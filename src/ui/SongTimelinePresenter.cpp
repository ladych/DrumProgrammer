#include "ui/SongTimelinePresenter.h"

#include "model/SongLayout.h"

#include <algorithm>
#include <utility>

namespace drumprog::ui
{

SongTimelinePresenter::SongTimelinePresenter(juce::ValueTree project,
                                             juce::UndoManager& undoManager,
                                             ActivePattern& active)
    : project_(std::move(project)), undoManager_(undoManager), active_(active),
      listener_(project_, [this] { ++changeCount_; })
{
}

std::vector<SongBlockView> SongTimelinePresenter::blocks() const
{
    const auto project = this->project();
    const auto song = project.song();
    const std::int64_t ticksPerBar = project.ticksPerBar();
    std::vector<SongBlockView> views;
    for (const auto& block : model::layoutSong(project))
    {
        const auto pattern = project.pattern(block.patternIndex);
        views.push_back({.patternIndex = block.patternIndex,
                         .startBar = static_cast<int>(block.startTick / ticksPerBar),
                         .lengthBars = static_cast<int>(block.lengthTicks / ticksPerBar),
                         .name = pattern.name(),
                         .colour = pattern.colour(),
                         .selected = song.entry(block.entryIndex).tree() == selected_});
    }
    return views;
}

int SongTimelinePresenter::songLengthBars() const
{
    const auto project = this->project();
    return static_cast<int>(model::songLengthTicks(model::layoutSong(project)) / project.ticksPerBar());
}

std::int64_t SongTimelinePresenter::ticksPerBar() const
{
    return project().ticksPerBar();
}

bool SongTimelinePresenter::isEmpty() const
{
    return project().song().numEntries() == 0;
}

int SongTimelinePresenter::blockAt(int bar) const
{
    const auto views = blocks();
    for (int index = static_cast<int>(views.size()) - 1; index >= 0; --index)
    {
        const auto& view = views[static_cast<std::size_t>(index)];
        if (bar >= view.startBar && bar < view.startBar + view.lengthBars)
            return index;
    }
    return -1;
}

void SongTimelinePresenter::select(int block)
{
    selected_ = entryOf(block);
}

int SongTimelinePresenter::selectedBlock() const
{
    const auto views = blocks();
    const auto found = std::ranges::find_if(views, &SongBlockView::selected);
    if (found == views.end())
        return -1;
    return static_cast<int>(std::distance(views.begin(), found));
}

void SongTimelinePresenter::insertPattern(int patternIndex, int bar)
{
    if (!isValidPattern(patternIndex))
        return;
    undoManager_.beginNewTransaction(juce::String::fromUTF8("Block einf\xc3\xbcgen"));
    add(patternIndex, bar);
}

void SongTimelinePresenter::appendActivePattern()
{
    insertPattern(active_.index(), songLengthBars());
}

void SongTimelinePresenter::removeSelected()
{
    if (!selected_.getParent().isValid())
        return;
    undoManager_.beginNewTransaction(juce::String::fromUTF8("Block l\xc3\xb6schen"));
    project().song().removeEntry(selected_.getParent().indexOf(selected_));
    selected_ = {};
}

void SongTimelinePresenter::clear()
{
    undoManager_.beginNewTransaction("Song leeren");
    project().song().clear();
}

void SongTimelinePresenter::open(int block)
{
    const auto views = blocks();
    if (block >= 0 && block < static_cast<int>(views.size()))
        active_.select(views[static_cast<std::size_t>(block)].patternIndex);
}

void SongTimelinePresenter::press(int block, int bar)
{
    select(block);
    drag_.reset();
    if (!selected_.isValid())
        return;
    const auto view = blocks().at(static_cast<std::size_t>(block));
    Drag drag;
    drag.entry = selected_;
    drag.patternIndex = view.patternIndex;
    drag.lengthBars = view.lengthBars;
    drag.originBar = view.startBar;
    drag.grabOffset = bar - view.startBar;
    drag.targetBar = view.startBar;
    drag_ = drag;
}

void SongTimelinePresenter::dragTo(int bar, bool duplicate)
{
    if (!drag_)
        return;
    drag_->targetBar = std::max(0, bar - drag_->grabOffset);
    drag_->duplicate = duplicate;
}

void SongTimelinePresenter::release()
{
    if (!drag_)
        return;
    const Drag drag = *drag_;
    drag_.reset();
    if (drag.targetBar == drag.originBar || !drag.entry.getParent().isValid())
        return;
    if (drag.duplicate)
    {
        undoManager_.beginNewTransaction("Block duplizieren");
        add(drag.patternIndex, drag.targetBar);
        return;
    }
    undoManager_.beginNewTransaction("Block verschieben");
    model::SongEntry{drag.entry, &undoManager_}.setStartBar(drag.targetBar);
}

void SongTimelinePresenter::hoverPattern(int patternIndex, int bar)
{
    hover_.reset();
    if (!isValidPattern(patternIndex))
        return;
    hover_ = SongGhost{.patternIndex = patternIndex,
                       .startBar = std::max(0, bar),
                       .lengthBars = project().pattern(patternIndex).lengthBars()};
}

void SongTimelinePresenter::endHover()
{
    hover_.reset();
}

std::optional<SongGhost> SongTimelinePresenter::ghost() const
{
    if (drag_)
    {
        if (drag_->targetBar == drag_->originBar)
            return std::nullopt;
        return SongGhost{.patternIndex = drag_->patternIndex,
                         .startBar = drag_->targetBar,
                         .lengthBars = drag_->lengthBars,
                         .duplicate = drag_->duplicate};
    }
    return hover_;
}

model::Project SongTimelinePresenter::project() const
{
    return {project_, &undoManager_};
}

juce::ValueTree SongTimelinePresenter::entryOf(int block) const
{
    const auto layout = model::layoutSong(project());
    if (block < 0 || block >= static_cast<int>(layout.size()))
        return {};
    return project().song().entry(layout[static_cast<std::size_t>(block)].entryIndex).tree();
}

bool SongTimelinePresenter::isValidPattern(int patternIndex) const
{
    if (patternIndex < 0)
        return false;
    return patternIndex < project().numPatterns();
}

void SongTimelinePresenter::add(int patternIndex, int bar)
{
    const auto entry = project().song().addEntry(project().pattern(patternIndex).id(), std::max(0, bar));
    selected_ = entry.tree();
}

} // namespace drumprog::ui
