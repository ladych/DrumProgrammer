#include "app/PianoRollView.h"

#include "app/Dialogs.h"
#include "app/Theme.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace drumprog::app
{
namespace
{

constexpr int kTimerHz = 30;
constexpr int kHeaderHeight = 34;
constexpr int kRowsWidth = 190;
constexpr int kLaneHeight = 110;
constexpr int kLaneMinimum = 50;
constexpr int kNotesMinimum = 120;
constexpr int kLegendWidth = 210;
constexpr int kLengthBoxWidth = 84;
constexpr int kHeaderButton = PianoRollToolbar::kButtonSize;
constexpr int kScrollBarSize = 12;
constexpr double kEndGrabPixels = 6.0;
constexpr double kVelocityGrabPixels = 5.0;
constexpr double kZoomStep = 1.25;

const juce::Colour kGridNote{0xFFF59E0B};
const juce::Colour kLiveNote{0xFFA78BFA};
const juce::Colour kPlayhead{0xFF22C55E};

juce::String utf8(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

juce::Colour noteColour(model::NoteOrigin origin)
{
    return origin == model::NoteOrigin::live ? kLiveNote : kGridNote;
}

/// Bar lines are drawn strongest, beat lines weaker, grid steps faintest (F-PR-08).
float lineAlpha(ui::GridLineKind kind)
{
    if (kind == ui::GridLineKind::bar)
        return 0.55F;
    if (kind == ui::GridLineKind::beat)
        return 0.25F;
    return 0.1F;
}

ui::PointerModifiers modifiersOf(const juce::MouseEvent& event)
{
    return {.ctrl = event.mods.isCommandDown(), .alt = event.mods.isAltDown()};
}

} // namespace

// ----- Note area ----------------------------------------------------------------------------------

class PianoRollView::NoteArea final : public juce::Component
{
public:
    NoteArea(PianoRollView& owner) : owner_(owner) {}

    void paint(juce::Graphics& g) override
    {
        paintBackground(g);
        paintGrid(g);
        for (const auto& note : owner_.presenter_.notes())
            paintNote(g, note);
        paintGhosts(g);
        paintSelectionRect(g);
        paintPlayhead(g);
    }

    void mouseMove(const juce::MouseEvent& event) override { updateCursor(event); }

    void mouseDown(const juce::MouseEvent& event) override
    {
        const auto button = event.mods.isPopupMenu() ? ui::PointerButton::right : ui::PointerButton::left;
        owner_.presenter_.mouseDown(pointOf(event), button, modifiersOf(event), tolerance());
        mousePosition_ = event.position;
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        owner_.presenter_.mouseDrag(pointOf(event), modifiersOf(event));
        mousePosition_ = event.position;
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        owner_.presenter_.mouseUp(pointOf(event), modifiersOf(event));
        updateCursor(event);
    }

private:
    [[nodiscard]] ui::GridPoint pointOf(const juce::MouseEvent& event) const
    {
        return {owner_.geometry_.tickAt(event.position.x), owner_.geometry_.rowAt(event.position.y)};
    }

    [[nodiscard]] std::int64_t tolerance() const { return owner_.geometry_.ticksFor(kEndGrabPixels); }

    void updateCursor(const juce::MouseEvent& event)
    {
        if (owner_.presenter_.tool() == ui::PianoRollTool::erase)
        {
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
            return;
        }
        const auto hit = owner_.presenter_.hitAt(pointOf(event), tolerance());
        if (hit == ui::NoteHit::end)
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else if (hit == ui::NoteHit::body)
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        else
            setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    [[nodiscard]] juce::Rectangle<float> rectOf(int row, std::int64_t start, std::int64_t length) const
    {
        const auto& geometry = owner_.geometry_;
        const auto x = static_cast<float>(geometry.xOf(start));
        const auto width = static_cast<float>(geometry.xOf(start + length)) - x;
        return {x,
                static_cast<float>(geometry.yOf(row)),
                std::max(width, 3.0F),
                static_cast<float>(geometry.rowHeight())};
    }

    void paintBackground(juce::Graphics& g) const
    {
        const auto& geometry = owner_.geometry_;
        const auto background = findColour(juce::ResizableWindow::backgroundColourId);
        g.fillAll(background.darker(0.4F));
        const auto lane = owner_.presenter_.laneRow();
        for (int row = geometry.firstVisibleRow(); row <= geometry.lastVisibleRow(); ++row)
        {
            const auto rowArea = juce::Rectangle<float>(0.0F,
                                                        static_cast<float>(geometry.yOf(row)),
                                                        static_cast<float>(getWidth()),
                                                        static_cast<float>(geometry.rowHeight()));
            g.setColour(lane == row ? kGridNote.withAlpha(0.12F)
                                    : background.darker(row % 2 == 0 ? 0.2F : 0.3F));
            g.fillRect(rowArea);
        }
        const auto end = static_cast<float>(geometry.xOf(owner_.presenter_.lengthTicks()));
        g.setColour(juce::Colours::black.withAlpha(0.35F));
        g.fillRect(juce::Rectangle<float>(
            end, 0.0F, static_cast<float>(getWidth()) - end, static_cast<float>(getHeight())));
    }

    void paintGrid(juce::Graphics& g) const
    {
        const auto& presenter = owner_.presenter_;
        const auto lines = owner_.geometry_.gridLines(
            presenter.ticksPerBar(), presenter.ticksPerBeat(), presenter.gridStepTicks());
        for (const auto& line : lines)
        {
            g.setColour(juce::Colours::white.withAlpha(lineAlpha(line.kind)));
            g.fillRect(juce::Rectangle<float>(static_cast<float>(owner_.geometry_.xOf(line.tick)),
                                              0.0F,
                                              line.kind == ui::GridLineKind::bar ? 2.0F : 1.0F,
                                              static_cast<float>(getHeight())));
        }
    }

    void paintNote(juce::Graphics& g, const ui::NoteView& note) const
    {
        const auto area = rectOf(note.row, note.start, note.length).reduced(0.5F, 2.0F);
        if (!g.getClipBounds().toFloat().intersects(area))
            return;
        g.setColour(noteColour(note.origin));
        g.fillRoundedRectangle(area, 3.0F);
        if (note.selected)
        {
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(area, 3.0F, 2.0F);
        }
    }

    void paintGhosts(juce::Graphics& g) const
    {
        const auto ghosts = owner_.presenter_.ghosts();
        if (ghosts.empty())
            return;
        g.setColour(juce::Colours::white.withAlpha(0.8F));
        for (const auto& ghost : ghosts)
        {
            const auto area = rectOf(ghost.row, ghost.start, ghost.length).reduced(0.5F, 2.0F);
            constexpr std::array kDashes{4.0F, 3.0F};
            juce::Path outline;
            outline.addRectangle(area);
            juce::Path dashed;
            juce::PathStrokeType(1.0F).createDashedStroke(
                dashed, outline, kDashes.data(), static_cast<int>(kDashes.size()));
            g.fillPath(dashed);
        }
        const auto tooltip = utf8(owner_.presenter_.dragTooltip());
        const auto box =
            juce::Rectangle<float>(mousePosition_.x + 12.0F, mousePosition_.y - 26.0F, 90.0F, 20.0F);
        g.setColour(juce::Colours::black.withAlpha(0.8F));
        g.fillRoundedRectangle(box, 3.0F);
        g.setColour(juce::Colours::white);
        g.drawText(tooltip, box, juce::Justification::centred);
    }

    void paintSelectionRect(juce::Graphics& g) const
    {
        const auto rect = owner_.presenter_.selectionRect();
        if (!rect)
            return;
        const auto& geometry = owner_.geometry_;
        const auto area =
            juce::Rectangle<float>::leftTopRightBottom(static_cast<float>(geometry.xOf(rect->startTick)),
                                                       static_cast<float>(geometry.yOf(rect->firstRow)),
                                                       static_cast<float>(geometry.xOf(rect->endTick)),
                                                       static_cast<float>(geometry.yOf(rect->lastRow + 1)));
        g.setColour(juce::Colours::white.withAlpha(0.12F));
        g.fillRect(area);
        g.setColour(juce::Colours::white.withAlpha(0.6F));
        g.drawRect(area, 1.0F);
    }

    void paintPlayhead(juce::Graphics& g) const
    {
        if (!owner_.playhead_)
            return;
        g.setColour(kPlayhead);
        g.fillRect(juce::Rectangle<float>(static_cast<float>(owner_.geometry_.xOf(*owner_.playhead_)) - 1.0F,
                                          0.0F,
                                          2.0F,
                                          static_cast<float>(getHeight())));
    }

    PianoRollView& owner_;
    juce::Point<float> mousePosition_;
};

// ----- Velocity lane ------------------------------------------------------------------------------

class PianoRollView::VelocityLane final : public juce::Component
{
public:
    VelocityLane(PianoRollView& owner) : owner_(owner) {}

    void paint(juce::Graphics& g) override
    {
        g.fillAll(findColour(juce::ResizableWindow::backgroundColourId).darker(0.3F));
        const auto row = owner_.presenter_.laneRow();
        if (!row)
            return;
        const auto height = static_cast<double>(getHeight());
        for (const auto& note : owner_.presenter_.notes())
        {
            if (note.row != *row)
                continue;
            const auto x = static_cast<float>(owner_.geometry_.xOf(note.start));
            const auto top = static_cast<float>(ui::PianoRollPresenter::yOfVelocity(note.velocity, height));
            g.setColour(noteColour(note.origin));
            g.fillRect(juce::Rectangle<float>(x, top, 3.0F, static_cast<float>(height) - top));
            g.fillEllipse(x - 2.5F, top - 4.0F, 8.0F, 8.0F);
            if (note.selected)
            {
                g.setColour(juce::Colours::white);
                g.drawEllipse(x - 2.5F, top - 4.0F, 8.0F, 8.0F, 1.5F);
            }
        }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        owner_.presenter_.velocityDragBegin(owner_.geometry_.tickAt(event.position.x),
                                            velocityAt(event),
                                            owner_.geometry_.ticksFor(kVelocityGrabPixels));
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        owner_.presenter_.velocityDrag(owner_.geometry_.tickAt(event.position.x), velocityAt(event));
    }

private:
    [[nodiscard]] int velocityAt(const juce::MouseEvent& event) const
    {
        return ui::PianoRollPresenter::velocityAt(event.position.y, getHeight());
    }

    PianoRollView& owner_;
};

// ----- Piano roll ---------------------------------------------------------------------------------

PianoRollView::PianoRollView(ui::PianoRollPresenter& presenter,
                             ui::TransportPresenter& transport,
                             ui::PatternListPresenter& patterns)
    : presenter_(presenter), transport_(transport), patterns_(patterns),
      geometry_(presenter.ticksPerQuarter()), laneHeight_(kLaneHeight, kLaneMinimum),
      noteArea_(std::make_unique<NoteArea>(*this)), velocityLane_(std::make_unique<VelocityLane>(*this)),
      snapControls_(presenter), toolbar_(presenter, transport)
{
    for (auto* component : std::initializer_list<juce::Component*>{noteArea_.get(),
                                                                   velocityLane_.get(),
                                                                   &horizontalBar_,
                                                                   &verticalBar_,
                                                                   &zoomOutButton_,
                                                                   &zoomInButton_,
                                                                   &lengthBox_,
                                                                   &lengthLabel_,
                                                                   &snapControls_,
                                                                   &toolbar_,
                                                                   &laneSplitter_})
    {
        component->setWantsKeyboardFocus(false);
        addAndMakeVisible(component);
    }
    horizontalBar_.addListener(this);
    verticalBar_.addListener(this);
    horizontalBar_.setAutoHide(false);
    verticalBar_.setAutoHide(false);
    zoomOutButton_.onClick = [this] { zoom(1.0 / kZoomStep); };
    zoomInButton_.onClick = [this] { zoom(kZoomStep); };
    setUpLengthBox();
    laneSplitter_.setDragHandlers(
        [this] { laneHeight_.beginDrag(getHeight() - kHeaderHeight, kNotesMinimum + kScrollBarSize); },
        [this](int offset) { dragLane(offset); });
    contentChanged();
    startTimerHz(kTimerHz);
}

PianoRollView::~PianoRollView()
{
    stopTimer();
    horizontalBar_.removeListener(this);
    verticalBar_.removeListener(this);
}

void PianoRollView::paint(juce::Graphics& g)
{
    const auto background = findColour(juce::ResizableWindow::backgroundColourId);
    g.fillAll(background);
    g.setColour(findColour(juce::Label::textColourId));
    g.setFont(juce::FontOptions(15.0F, juce::Font::bold));
    g.drawText(headerText_, headerTextArea_, juce::Justification::centredLeft);
    if (!legendArea_.isEmpty())
        paintLegend(g, legendArea_);
    paintRows(g);
    g.setFont(juce::FontOptions(13.0F));
    g.setColour(findColour(juce::Label::textColourId));
    auto lane = laneHeaderArea_.reduced(8, 6);
    g.drawText("Velocity", lane.removeFromTop(18), juce::Justification::topLeft);
    g.setColour(findColour(juce::Label::textColourId).withAlpha(0.6F));
    g.drawText(utf8(presenter_.laneLabel()), lane.removeFromTop(18), juce::Justification::topLeft);
}

void PianoRollView::resized()
{
    auto area = getLocalBounds();
    headerArea_ = area.removeFromTop(kHeaderHeight);
    layOutHeader();

    const int laneHeight = laneHeight_.size(area.getHeight(), kNotesMinimum + kScrollBarSize);
    auto lane = area.removeFromBottom(laneHeight);
    laneSplitter_.setBounds(lane.removeFromTop(Splitter::kThickness));
    laneHeaderArea_ = lane.removeFromLeft(kRowsWidth);
    lane.removeFromRight(kScrollBarSize);
    velocityLane_->setBounds(lane);

    horizontalBar_.setBounds(
        area.removeFromBottom(kScrollBarSize).withTrimmedLeft(kRowsWidth).withTrimmedRight(kScrollBarSize));
    verticalBar_.setBounds(area.removeFromRight(kScrollBarSize));
    rowsArea_ = area.removeFromLeft(kRowsWidth);
    noteArea_->setBounds(area);
    geometry_.setViewSize(area.getWidth(), area.getHeight());
    updateScrollBars();
}

/// Header from the left: general information, snap controls; from the right: legend, recording mode,
/// tools and zoom (AP14). The legend is left out when the piano roll is too narrow for all of it.
void PianoRollView::layOutHeader()
{
    headerText_ = utf8(presenter_.headerText());
    const auto font = juce::Font(juce::FontOptions(15.0F, juce::Font::bold));
    const int textWidth =
        static_cast<int>(std::ceil(juce::GlyphArrangement::getStringWidth(font, headerText_))) + 16;
    constexpr int kButtonGroups = 2 * kHeaderButton + 2 + PianoRollToolbar::kGroupGap +
                                  PianoRollToolbar::kPreferredWidth + PianoRollToolbar::kGroupGap;
    auto header = headerArea_.reduced(8, 0);
    const bool withLegend = header.getWidth() >= textWidth + kLengthBoxWidth + PianoRollToolbar::kGroupGap +
                                                     SnapControls::kPreferredWidth + kButtonGroups +
                                                     kLegendWidth;
    legendArea_ = withLegend ? header.removeFromRight(kLegendWidth) : juce::Rectangle<int>{};

    auto buttons = header.withSizeKeepingCentre(header.getWidth(), kHeaderButton);
    toolbar_.setBounds(buttons.removeFromRight(PianoRollToolbar::kPreferredWidth));
    buttons.removeFromRight(PianoRollToolbar::kGroupGap);
    zoomInButton_.setBounds(buttons.removeFromRight(kHeaderButton));
    buttons.removeFromRight(2);
    zoomOutButton_.setBounds(buttons.removeFromRight(kHeaderButton));
    header.setRight(zoomOutButton_.getX() - PianoRollToolbar::kGroupGap);

    headerTextArea_ = header.removeFromLeft(std::min(textWidth, header.getWidth()));
    auto length =
        header.removeFromLeft(kLengthBoxWidth).withSizeKeepingCentre(kLengthBoxWidth, kHeaderButton);
    lengthBox_.setBounds(length.removeFromLeft(36));
    lengthLabel_.setBounds(length);
    header.removeFromLeft(PianoRollToolbar::kGroupGap);
    snapControls_.setBounds(header.removeFromLeft(SnapControls::kPreferredWidth)
                                .withSizeKeepingCentre(SnapControls::kPreferredWidth, kHeaderButton));
}

/// Length of the shown pattern, typed right in the header (F-PR-09, AP14). A wrong number is marked red
/// and not taken; Return or leaving the field takes a right one, Escape restores the length.
void PianoRollView::setUpLengthBox()
{
    lengthBox_.setWantsKeyboardFocus(true);
    lengthBox_.setInputRestrictions(2, "0123456789");
    lengthBox_.setJustification(juce::Justification::centred);
    lengthBox_.setTooltip(juce::String::fromUTF8("Länge des Patterns in Takten (1–64), Eingabe mit Enter"));
    lengthBox_.onTextChange = [this]
    {
        markInput(lengthBox_,
                  ui::PatternListPresenter::parseBars(lengthBox_.getText().toStdString()).has_value());
    };
    lengthBox_.onReturnKey = [] { unfocusAllComponents(); };
    lengthBox_.onEscapeKey = [this]
    {
        lengthBox_.setText(juce::String(patterns_.lengthBars(patterns_.selectedIndex())), true);
        unfocusAllComponents();
    };
    lengthBox_.onFocusLost = [this] { commitLength(); };
    lengthLabel_.setColour(juce::Label::textColourId, theme::kTextDim);
    refreshLengthBox();
}

void PianoRollView::commitLength()
{
    const int index = patterns_.selectedIndex();
    const auto bars = ui::PatternListPresenter::parseBars(lengthBox_.getText().toStdString());
    if (bars && *bars != patterns_.lengthBars(index))
        patterns_.setLengthBars(index, *bars);
    lengthBox_.clear();
    refreshLengthBox();
}

void PianoRollView::refreshLengthBox()
{
    if (lengthBox_.hasKeyboardFocus(false))
        return;
    const juce::String bars(patterns_.lengthBars(patterns_.selectedIndex()));
    if (bars != lengthBox_.getText())
    {
        lengthBox_.setText(bars, false);
        markInput(lengthBox_, true);
    }
}

void PianoRollView::dragLane(int offset)
{
    // The splitter is above the lane: dragging down makes the lane smaller.
    laneHeight_.dragTo(-offset, getHeight() - kHeaderHeight, kNotesMinimum + kScrollBarSize);
    resized();
    repaint();
}

void PianoRollView::timerCallback()
{
    if (presenter_.changeCount() != seenChangeCount_)
        contentChanged();
    refreshLengthBox();
    const auto playhead = transport_.playheadTick();
    if (playhead != playhead_)
    {
        playhead_ = playhead;
        noteArea_->repaint();
    }
}

void PianoRollView::scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart)
{
    if (scrollBar == &horizontalBar_)
        geometry_.scrollBy(newRangeStart - geometry_.scrollX(), 0.0);
    else
        geometry_.scrollBy(0.0, newRangeStart - geometry_.scrollY());
    repaint();
}

void PianoRollView::mouseDown(const juce::MouseEvent& event)
{
    // A click on a row name chooses the row of the velocity lane.
    if (rowsArea_.contains(event.getPosition()))
        presenter_.setLaneRow(geometry_.rowAt(event.position.y - static_cast<float>(rowsArea_.getY())));
}

void PianoRollView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    constexpr double kWheelPixels = 120.0;
    const double delta = wheel.deltaY != 0.0F ? wheel.deltaY : wheel.deltaX;
    const auto inArea = event.getEventRelativeTo(noteArea_.get()).position;
    if (event.mods.isCommandDown() && event.mods.isShiftDown())
        geometry_.zoomVertically(delta > 0.0 ? kZoomStep : 1.0 / kZoomStep, inArea.y);
    else if (event.mods.isCommandDown())
        geometry_.zoomHorizontally(delta > 0.0 ? kZoomStep : 1.0 / kZoomStep, inArea.x);
    else if (event.mods.isShiftDown() || wheel.deltaX != 0.0F)
        geometry_.scrollBy(-delta * kWheelPixels, 0.0);
    else
        geometry_.scrollBy(0.0, -delta * kWheelPixels);
    updateScrollBars();
    repaint();
}

void PianoRollView::contentChanged()
{
    seenChangeCount_ = presenter_.changeCount();
    refreshLengthBox();
    if (utf8(presenter_.headerText()) != headerText_)
    {
        layOutHeader();
        repaint(headerArea_);
    }
    geometry_.setContent(presenter_.lengthTicks(), presenter_.numRows());
    updateScrollBars();
    repaint();
}

void PianoRollView::updateScrollBars()
{
    const double width = geometry_.xOf(presenter_.lengthTicks()) + geometry_.scrollX();
    const double height = geometry_.yOf(presenter_.numRows()) + geometry_.scrollY();
    horizontalBar_.setRangeLimits(0.0, std::max(width, 1.0), juce::dontSendNotification);
    horizontalBar_.setCurrentRange(geometry_.scrollX(), noteArea_->getWidth(), juce::dontSendNotification);
    verticalBar_.setRangeLimits(0.0, std::max(height, 1.0), juce::dontSendNotification);
    verticalBar_.setCurrentRange(geometry_.scrollY(), noteArea_->getHeight(), juce::dontSendNotification);
}

void PianoRollView::zoom(double factor)
{
    geometry_.zoomHorizontally(factor, 0.0);
    updateScrollBars();
    repaint();
}

void PianoRollView::paintRows(juce::Graphics& g) const
{
    g.saveState();
    g.reduceClipRegion(rowsArea_);
    g.setFont(juce::FontOptions(13.0F));
    const auto text = findColour(juce::Label::textColourId);
    const auto lane = presenter_.laneRow();
    for (int row = geometry_.firstVisibleRow(); row <= geometry_.lastVisibleRow(); ++row)
    {
        const auto& shown = presenter_.rows()[static_cast<std::size_t>(row)];
        auto area = juce::Rectangle<int>(rowsArea_.getX(),
                                         rowsArea_.getY() + static_cast<int>(geometry_.yOf(row)),
                                         rowsArea_.getWidth(),
                                         geometry_.rowHeight())
                        .reduced(6, 0);
        if (lane == row)
        {
            g.setColour(kGridNote.withAlpha(0.15F));
            g.fillRect(area);
        }
        g.setColour(text.withAlpha(0.5F));
        g.drawText(juce::String(shown.midiNote), area.removeFromLeft(26), juce::Justification::centredLeft);
        const auto badge = area.removeFromRight(24).reduced(1, 3);
        g.drawRect(badge);
        g.drawText(utf8(presenter_.rowKey(row)), badge, juce::Justification::centred);
        g.setColour(text);
        g.drawText(utf8(shown.name), area, juce::Justification::centredLeft, true);
    }
    g.restoreState();
}

void PianoRollView::paintLegend(juce::Graphics& g, juce::Rectangle<int> area) const
{
    g.setFont(juce::FontOptions(13.0F));
    for (const auto& [colour, label] : {std::pair{kGridNote, juce::String("gerastert")},
                                        std::pair{kLiveNote, juce::String("frei eingespielt")}})
    {
        auto item = area.removeFromLeft(label.length() * 7 + 30);
        g.setColour(colour);
        g.fillRect(item.removeFromLeft(14).withSizeKeepingCentre(10, 10));
        g.setColour(findColour(juce::Label::textColourId));
        g.drawText(label, item.withTrimmedLeft(4), juce::Justification::centredLeft);
    }
}

} // namespace drumprog::app
