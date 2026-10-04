#include "app/SongTimelineView.h"

#include <algorithm>
#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 30;
constexpr int kHeaderHeight = 28;
constexpr int kRulerHeight = 20;
constexpr int kLabelWidth = 120;
constexpr int kScrollBarSize = 12;

const juce::Colour kPlayhead{0xFF22C55E};

juce::String utf8(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

juce::Colour colourOf(const std::string& colour)
{
    return juce::Colour::fromString("FF" + utf8(colour).trimCharactersAtStart("#"));
}

/// Pattern index of a drag from the pattern list, -1 for anything else.
int patternOf(const juce::DragAndDropTarget::SourceDetails& details)
{
    const auto description = details.description.toString();
    if (!description.startsWith(SongTimelineView::kPatternDragPrefix))
        return -1;
    return description.fromFirstOccurrenceOf(SongTimelineView::kPatternDragPrefix, false, false)
        .getIntValue();
}

} // namespace

// ----- Drums track --------------------------------------------------------------------------------

class SongTimelineView::Track final : public juce::Component, public juce::DragAndDropTarget
{
public:
    explicit Track(SongTimelineView& owner) : owner_(owner) {}

    void paint(juce::Graphics& g) override
    {
        g.fillAll(findColour(juce::ResizableWindow::backgroundColourId).darker(0.25F));
        paintBarLines(g);
        for (const auto& block : owner_.presenter_.blocks())
            paintBlock(g, block);
        paintGhost(g);
        paintPlayhead(g);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        const int bar = barAt(event);
        const int block = presenter().blockAt(bar);
        presenter().press(block, bar);
        if (event.mods.isPopupMenu() && block >= 0)
            showMenu(block);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        presenter().dragTo(barAt(event), event.mods.isAltDown());
        repaint();
    }

    void mouseUp(const juce::MouseEvent& /*event*/) override
    {
        presenter().release();
        repaint();
    }

    void mouseDoubleClick(const juce::MouseEvent& event) override
    {
        presenter().open(presenter().blockAt(barAt(event)));
    }

    void mouseMove(const juce::MouseEvent& event) override
    {
        const bool onBlock = presenter().blockAt(barAt(event)) >= 0;
        setMouseCursor(onBlock ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    }

    bool isInterestedInDragSource(const SourceDetails& details) override { return patternOf(details) >= 0; }

    void itemDragMove(const SourceDetails& details) override
    {
        presenter().hoverPattern(patternOf(details), barAt(details.localPosition.x));
        repaint();
    }

    void itemDragExit(const SourceDetails& /*details*/) override
    {
        presenter().endHover();
        repaint();
    }

    void itemDropped(const SourceDetails& details) override
    {
        presenter().endHover();
        presenter().insertPattern(patternOf(details), barAt(details.localPosition.x));
        repaint();
    }

private:
    [[nodiscard]] ui::SongTimelinePresenter& presenter() const { return owner_.presenter_; }

    [[nodiscard]] int barAt(const juce::MouseEvent& event) const
    {
        return owner_.geometry_.barAt(event.position.x);
    }

    [[nodiscard]] int barAt(int x) const { return owner_.geometry_.barAt(static_cast<double>(x)); }

    [[nodiscard]] juce::Rectangle<float> blockArea(int startBar, int lengthBars) const
    {
        const auto& geometry = owner_.geometry_;
        const auto x = static_cast<float>(geometry.xOfBar(startBar));
        const auto right = static_cast<float>(geometry.xOfBar(startBar + lengthBars));
        return juce::Rectangle<float>(x, 4.0F, right - x, static_cast<float>(getHeight()) - 8.0F)
            .reduced(1.0F, 0.0F);
    }

    void paintBarLines(juce::Graphics& g) const
    {
        const auto& geometry = owner_.geometry_;
        const auto line = findColour(juce::Label::textColourId);
        const int step = geometry.labelStep();
        for (int bar = geometry.firstVisibleBar(); bar <= geometry.lastVisibleBar() + 1; ++bar)
        {
            g.setColour(line.withAlpha(bar % step == 0 ? 0.25F : 0.08F));
            g.fillRect(static_cast<float>(geometry.xOfBar(bar)), 0.0F, 1.0F, static_cast<float>(getHeight()));
        }
    }

    void paintBlock(juce::Graphics& g, const ui::SongBlockView& block) const
    {
        const auto area = blockArea(block.startBar, block.lengthBars);
        if (area.getRight() < 0.0F || area.getX() > static_cast<float>(getWidth()))
            return;
        g.setColour(colourOf(block.colour));
        g.fillRoundedRectangle(area, 3.0F);
        if (block.selected)
        {
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(area.reduced(1.0F), 3.0F, 2.0F);
        }
        g.setColour(juce::Colours::black.withAlpha(0.85F));
        auto text = area.reduced(5.0F, 3.0F);
        g.setFont(juce::FontOptions(13.0F, juce::Font::bold));
        g.drawText(utf8(block.name), text.removeFromTop(16.0F), juce::Justification::topLeft, true);
        g.setFont(juce::FontOptions(11.0F));
        g.drawText(
            juce::String(block.lengthBars) + " T.", text.removeFromTop(14.0F), juce::Justification::topLeft);
    }

    void paintGhost(juce::Graphics& g) const
    {
        const auto ghost = presenter().ghost();
        if (!ghost)
            return;
        const auto area = blockArea(ghost->startBar, ghost->lengthBars);
        g.setColour(juce::Colours::white.withAlpha(0.15F));
        g.fillRoundedRectangle(area, 3.0F);
        g.setColour(juce::Colours::white);
        const float dashes[] = {4.0F, 3.0F};
        juce::Path outline;
        outline.addRoundedRectangle(area, 3.0F);
        juce::PathStrokeType(1.5F).createDashedStroke(outline, outline, dashes, 2);
        g.fillPath(outline);
        if (ghost->duplicate)
            g.drawText("+", area.reduced(4.0F), juce::Justification::topRight);
    }

    void paintPlayhead(juce::Graphics& g) const
    {
        if (!owner_.playhead_)
            return;
        const double bar =
            static_cast<double>(*owner_.playhead_) / static_cast<double>(presenter().ticksPerBar());
        g.setColour(kPlayhead);
        g.fillRect(static_cast<float>(owner_.geometry_.xOfBar(bar)) - 1.0F,
                   0.0F,
                   2.0F,
                   static_cast<float>(getHeight()));
    }

    void showMenu(int block)
    {
        juce::PopupMenu menu;
        menu.addItem(1,
                     juce::String::fromUTF8("Pattern im Piano-Roll \xc3\xb6"
                                            "ffnen"));
        menu.addItem(2, juce::String::fromUTF8("Block l\xc3\xb6schen"));
        menu.showMenuAsync(juce::PopupMenu::Options{},
                           [this, block](int result)
                           {
                               if (result == 1)
                                   presenter().open(block);
                               else if (result == 2)
                                   presenter().removeSelected();
                           });
    }

    SongTimelineView& owner_;
};

// ----- Timeline -----------------------------------------------------------------------------------

SongTimelineView::SongTimelineView(ui::SongTimelinePresenter& presenter, ui::TransportPresenter& transport)
    : presenter_(presenter), transport_(transport), track_(std::make_unique<Track>(*this))
{
    for (auto* component :
         std::initializer_list<juce::Component*>{track_.get(), &scrollBar_, &zoomOutButton_, &zoomInButton_})
    {
        component->setWantsKeyboardFocus(false);
        component->setMouseClickGrabsKeyboardFocus(false);
        addAndMakeVisible(component);
    }
    scrollBar_.addListener(this);
    scrollBar_.setAutoHide(false);
    zoomOutButton_.setTooltip("Timeline verkleinern");
    zoomInButton_.setTooltip(juce::String::fromUTF8("Timeline vergr\xc3\xb6\xc3\x9f"
                                                    "ern"));
    zoomOutButton_.onClick = [this] { zoom(false); };
    zoomInButton_.onClick = [this] { zoom(true); };
    contentChanged();
    startTimerHz(kTimerHz);
}

SongTimelineView::~SongTimelineView()
{
    stopTimer();
    scrollBar_.removeListener(this);
}

void SongTimelineView::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
    const auto text = findColour(juce::Label::textColourId);
    auto header = headerArea_.reduced(8, 0);
    header.removeFromRight(zoomOutButton_.getWidth() + zoomInButton_.getWidth() + 8);
    g.setColour(text);
    g.setFont(juce::FontOptions(15.0F, juce::Font::bold));
    const juce::String title{"Song-Arrangement"};
    g.drawText(title, header.removeFromLeft(150), juce::Justification::centredLeft);
    g.setColour(text.withAlpha(0.6F));
    g.setFont(juce::FontOptions(13.0F));
    g.drawText(juce::String::fromUTF8("Pattern-Bl\xc3\xb6"
                                      "cke per Drag & Drop \xc2\xb7 Alt-Ziehen dupliziert \xc2\xb7 "
                                      "Wiederholungen = Referenz auf dasselbe Pattern"),
               header,
               juce::Justification::centredLeft,
               true);

    auto label = trackLabelArea_.reduced(8, 6);
    g.setColour(text);
    g.drawText("Drums", label.removeFromTop(18), juce::Justification::topLeft);
    g.setColour(text.withAlpha(0.6F));
    g.drawText("Pattern-Kette", label.removeFromTop(18), juce::Justification::topLeft);
    paintRuler(g);
}

void SongTimelineView::resized()
{
    auto area = getLocalBounds();
    headerArea_ = area.removeFromTop(kHeaderHeight);
    auto zoomArea = headerArea_.removeFromRight(56).reduced(0, 3);
    zoomInButton_.setBounds(zoomArea.removeFromRight(26));
    zoomOutButton_.setBounds(zoomArea.removeFromRight(26));
    rulerArea_ = area.removeFromTop(kRulerHeight).withTrimmedLeft(kLabelWidth);
    scrollBar_.setBounds(area.removeFromBottom(kScrollBarSize).withTrimmedLeft(kLabelWidth));
    trackLabelArea_ = area.removeFromLeft(kLabelWidth);
    track_->setBounds(area);
    geometry_.setViewWidth(area.getWidth());
    updateScrollBar();
}

void SongTimelineView::timerCallback()
{
    if (presenter_.changeCount() != seenChangeCount_)
        contentChanged();
    const auto playhead = transport_.songPlayheadTick();
    if (playhead != playhead_)
    {
        playhead_ = playhead;
        track_->repaint();
    }
}

void SongTimelineView::scrollBarMoved(juce::ScrollBar* /*scrollBar*/, double newRangeStart)
{
    geometry_.scrollBy(newRangeStart - geometry_.scrollX());
    repaint();
}

void SongTimelineView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    constexpr double kWheelPixels = 120.0;
    const double delta = wheel.deltaY != 0.0F ? wheel.deltaY : wheel.deltaX;
    if (event.mods.isCommandDown())
    {
        zoom(delta > 0.0);
        return;
    }
    geometry_.scrollBy(-delta * kWheelPixels);
    updateScrollBar();
    repaint();
}

void SongTimelineView::contentChanged()
{
    seenChangeCount_ = presenter_.changeCount();
    geometry_.setSongLength(presenter_.songLengthBars());
    updateScrollBar();
    repaint();
}

void SongTimelineView::updateScrollBar()
{
    const double width =
        static_cast<double>(presenter_.songLengthBars() + ui::SongTimelineGeometry::kExtraBars) *
        geometry_.pixelsPerBar();
    scrollBar_.setRangeLimits(0.0, std::max(width, 1.0), juce::dontSendNotification);
    scrollBar_.setCurrentRange(geometry_.scrollX(), track_->getWidth(), juce::dontSendNotification);
    zoomInButton_.setEnabled(geometry_.canZoomIn());
    zoomOutButton_.setEnabled(geometry_.canZoomOut());
}

void SongTimelineView::zoom(bool in)
{
    if (in)
        geometry_.zoomIn();
    else
        geometry_.zoomOut();
    updateScrollBar();
    repaint();
}

void SongTimelineView::paintRuler(juce::Graphics& g) const
{
    g.saveState();
    g.reduceClipRegion(rulerArea_);
    g.setFont(juce::FontOptions(11.0F));
    g.setColour(findColour(juce::Label::textColourId).withAlpha(0.7F));
    const int step = geometry_.labelStep();
    for (int bar = geometry_.firstVisibleBar() / step * step; bar <= geometry_.lastVisibleBar(); bar += step)
    {
        const int x = rulerArea_.getX() + static_cast<int>(geometry_.xOfBar(bar));
        g.fillRect(x, rulerArea_.getBottom() - 6, 1, 6);
        g.drawText(juce::String(bar + 1),
                   x + 3,
                   rulerArea_.getY(),
                   40,
                   rulerArea_.getHeight() - 2,
                   juce::Justification::centredLeft);
    }
    g.restoreState();
}

} // namespace drumprog::app
