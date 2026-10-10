#include "app/SongTimelineView.h"

#include "app/Dialogs.h"
#include "io/Utf8Path.h"

#include <algorithm>
#include <array>
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
constexpr int kBackingHeight = 52;
constexpr int kThumbnailSamplesPerPoint = 512;

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
        owner_.paintPlayhead(g, getHeight());
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
        const std::array dashes{4.0F, 3.0F};
        juce::Path outline;
        outline.addRoundedRectangle(area, 3.0F);
        juce::PathStrokeType(1.5F).createDashedStroke(
            outline, outline, dashes.data(), static_cast<int>(dashes.size()));
        g.fillPath(outline);
        if (ghost->duplicate)
            g.drawText("+", area.reduced(4.0F), juce::Justification::topRight);
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

// ----- Backing track lane -------------------------------------------------------------------------

class SongTimelineView::BackingLane final : public juce::Component
{
public:
    BackingLane(SongTimelineView& owner, juce::AudioFormatManager& formats)
        : owner_(owner), thumbnail_(kThumbnailSamplesPerPoint, formats, cache_)
    {
    }

    /// Call when the presenter changed: follows a new file.
    void update()
    {
        const auto file = backing().isMissing() ? std::filesystem::path{} : backing().file();
        if (file == file_)
            return;
        file_ = file;
        if (file_.empty())
            thumbnail_.clear();
        else
            thumbnail_.setSource(new juce::FileInputSource(juce::File{utf8(io::utf8FromPath(file_))}));
        repaint();
    }

    /// The thumbnail is built on a background thread; repaint until it is complete.
    void repaintWhileLoading()
    {
        if (!file_.empty() && !thumbnail_.isFullyLoaded())
            repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(findColour(juce::ResizableWindow::backgroundColourId).darker(0.15F));
        const auto text = findColour(juce::Label::textColourId);
        if (!backing().hasTrack())
        {
            g.setColour(text.withAlpha(0.45F));
            g.drawText(juce::String::fromUTF8(
                           "Kein Backing-Track \xc2\xb7 Rechtsklick oder Men\xc3\xbc Audio zum Laden"),
                       getLocalBounds().reduced(8, 0),
                       juce::Justification::centredLeft);
            return;
        }
        paintWaveform(g);
        owner_.paintPlayhead(g, getHeight());
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isPopupMenu())
        {
            showMenu();
            return;
        }
        dragStartX_ = event.position.x;
        backing().beginOffsetDrag();
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        backing().dragOffsetBy(static_cast<double>(event.position.x - dragStartX_) /
                               owner_.geometry_.pixelsPerBar());
        repaint();
    }

    void mouseUp(const juce::MouseEvent& /*event*/) override { backing().endOffsetDrag(); }

private:
    [[nodiscard]] ui::BackingTrackPresenter& backing() const { return owner_.backing_; }

    void paintWaveform(juce::Graphics& g)
    {
        const auto& geometry = owner_.geometry_;
        const double left = std::max(0.0, geometry.xOfBar(backing().startBar()));
        const double right = std::min(static_cast<double>(getWidth()), geometry.xOfBar(backing().endBar()));
        if (right <= left || file_.empty())
            return;
        const auto area = juce::Rectangle<double>(left, 4.0, right - left, getHeight() - 8.0).toNearestInt();
        g.setColour(juce::Colour{0xFF3B82F6}.withAlpha(0.25F));
        g.fillRect(area);
        g.setColour(juce::Colour{0xFF60A5FA});
        thumbnail_.drawChannels(g,
                                area,
                                backing().fileSecondsAtBar(geometry.exactBarAt(area.getX())),
                                backing().fileSecondsAtBar(geometry.exactBarAt(area.getRight())),
                                1.0F);
    }

    void showMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Backing-Track laden...");
        menu.addItem(2, "Versatz eingeben...", !backing().isMissing() && backing().hasTrack());
        menu.addItem(3,
                     juce::String::fromUTF8("Versatz zur\xc3\xbc"
                                            "cksetzen"),
                     !backing().isMissing() && backing().hasTrack());
        menu.addItem(4, "Backing-Track entfernen", backing().hasTrack());
        menu.showMenuAsync(juce::PopupMenu::Options{}, [this](int result) { onMenu(result); });
    }

    void onMenu(int result)
    {
        if (result == 1 && owner_.loadBackingTrack_)
            owner_.loadBackingTrack_();
        else if (result == 2)
            askForText("Versatz Backing-Track",
                       juce::String::fromUTF8("Millisekunden der Datei vor Takt 1 (negativ: Track beginnt "
                                              "sp\xc3\xa4ter)"),
                       juce::String(backing().offsetMs(), 1),
                       [this](const std::string& text)
                       { backing().setOffsetMs(juce::String::fromUTF8(text.c_str()).getDoubleValue()); });
        else if (result == 3)
            backing().setOffsetMs(0.0);
        else if (result == 4)
            backing().remove();
    }

    SongTimelineView& owner_;
    juce::AudioThumbnailCache cache_{1};
    juce::AudioThumbnail thumbnail_;
    std::filesystem::path file_;
    float dragStartX_ = 0.0F;
};

// ----- Timeline -----------------------------------------------------------------------------------

SongTimelineView::SongTimelineView(ui::SongTimelinePresenter& presenter,
                                   ui::TransportPresenter& transport,
                                   ui::BackingTrackPresenter& backing,
                                   juce::AudioFormatManager& formats,
                                   std::function<void()> loadBackingTrack)
    : presenter_(presenter), transport_(transport), backing_(backing),
      loadBackingTrack_(std::move(loadBackingTrack)),
      backingLane_(std::make_unique<BackingLane>(*this, formats)), track_(std::make_unique<Track>(*this))
{
    for (auto* component : std::initializer_list<juce::Component*>{
             backingLane_.get(), track_.get(), &scrollBar_, &zoomOutButton_, &zoomInButton_})
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

    paintTrackLabels(g);
    paintRuler(g);
}

void SongTimelineView::paintTrackLabels(juce::Graphics& g) const
{
    const auto text = findColour(juce::Label::textColourId);
    auto backing = backingLabelArea_.reduced(8, 6);
    g.setColour(text);
    g.drawText("Backing", backing.removeFromTop(18), juce::Justification::topLeft);
    g.setColour(backing_.isMissing() ? juce::Colours::orangered : text.withAlpha(0.6F));
    g.setFont(juce::FontOptions(12.0F));
    g.drawText(utf8(backing_.label()), backing.removeFromTop(16), juce::Justification::topLeft, true);

    auto label = trackLabelArea_.reduced(8, 6);
    g.setFont(juce::FontOptions(15.0F));
    g.setColour(text);
    g.drawText("Drums", label.removeFromTop(18), juce::Justification::topLeft);
    g.setColour(text.withAlpha(0.6F));
    g.drawText("Pattern-Kette", label.removeFromTop(18), juce::Justification::topLeft);
}

void SongTimelineView::mouseDown(const juce::MouseEvent& event)
{
    // Click on the ruler: the song continues from this bar (F-BT-07).
    if (!rulerArea_.contains(event.getPosition()))
        return;
    const int bar = geometry_.barAt(event.position.x - static_cast<float>(rulerArea_.getX()));
    transport_.locate(bar * presenter_.ticksPerBar());
}

void SongTimelineView::paintPlayhead(juce::Graphics& g, int height) const
{
    if (!playhead_)
        return;
    const double bar = static_cast<double>(*playhead_) / static_cast<double>(presenter_.ticksPerBar());
    g.setColour(kPlayhead);
    g.fillRect(static_cast<float>(geometry_.xOfBar(bar)) - 1.0F, 0.0F, 2.0F, static_cast<float>(height));
}

int SongTimelineView::contentBars() const
{
    return std::max(presenter_.songLengthBars(), backing_.lengthBars());
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
    auto backingRow = area.removeFromTop(kBackingHeight);
    backingLabelArea_ = backingRow.removeFromLeft(kLabelWidth);
    backingLane_->setBounds(backingRow);
    trackLabelArea_ = area.removeFromLeft(kLabelWidth);
    track_->setBounds(area);
    geometry_.setViewWidth(area.getWidth());
    updateScrollBar();
}

void SongTimelineView::timerCallback()
{
    if (presenter_.changeCount() != seenChangeCount_ || backing_.changeCount() != seenBackingChangeCount_)
        contentChanged();
    backing_.tick();
    backingLane_->repaintWhileLoading();
    const auto playhead = transport_.songPlayheadTick();
    if (playhead != playhead_)
    {
        playhead_ = playhead;
        track_->repaint();
        backingLane_->repaint();
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
    seenBackingChangeCount_ = backing_.changeCount();
    backingLane_->update();
    geometry_.setSongLength(contentBars());
    updateScrollBar();
    repaint();
}

void SongTimelineView::updateScrollBar()
{
    const double width =
        static_cast<double>(contentBars() + ui::SongTimelineGeometry::kExtraBars) * geometry_.pixelsPerBar();
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
