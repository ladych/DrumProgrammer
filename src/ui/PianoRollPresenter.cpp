#include "ui/PianoRollPresenter.h"

#include "engine/KitDescription.h"
#include "engine/TempoMath.h"
#include "ui/MusicalTime.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace drumprog::ui
{
namespace
{

constexpr int kMaxVelocity = 127;
const juce::String kVelocityEdit{"Velocity"};
const juce::String kDeleteNotes = juce::String::fromUTF8("Noten l\xc3\xb6schen");
constexpr const char* kSeparator = " \xc2\xb7 ";
constexpr const char* kDash = "\xe2\x80\x93";

/// Where tick lies on the way from one tick to another, 0..1; 1 if both are the same.
double fractionOf(std::int64_t tick, std::int64_t from, std::int64_t to)
{
    if (from == to)
        return 1.0;
    return std::clamp(static_cast<double>(tick - from) / static_cast<double>(to - from), 0.0, 1.0);
}

std::string plural(int count, const char* one, const char* many)
{
    return std::to_string(count) + (count == 1 ? one : many);
}

} // namespace

PianoRollPresenter::PianoRollPresenter(juce::ValueTree project,
                                       juce::ValueTree globalKit,
                                       juce::UndoManager& undoManager,
                                       ActivePattern& active,
                                       std::function<std::string(int slotIndex)> keyLabel)
    : project_(std::move(project)), globalKit_(std::move(globalKit)), undoManager_(undoManager),
      active_(active), keyLabel_(std::move(keyLabel)), projectListener_(project_, [this] { refresh(); }),
      globalKitListener_(globalKit_, [this] { refresh(); })
{
    active_.addOnChange([this](int /*index*/) { refresh(); });
    refresh();
}

// ----- Rows and pattern ---------------------------------------------------------------------------

std::string PianoRollPresenter::rowKey(int row) const
{
    if (row < 0 || row >= numRows())
        return {};
    return keyLabel_(rowSlots_[static_cast<std::size_t>(row)]);
}

bool PianoRollPresenter::hasPattern() const
{
    return pattern().has_value();
}

std::string PianoRollPresenter::headerText() const
{
    const auto shown = pattern();
    if (!shown)
        return "Piano-Roll";
    const auto signature = project().timeSignature();
    std::string text = "Piano-Roll \xe2\x80\x94 " + shown->name();
    text += kSeparator + plural(shown->lengthBars(), " Takt", " Takte");
    text += kSeparator + std::to_string(signature.numerator) + "/" + std::to_string(signature.denominator);
    text += kSeparator + std::string{"Raster "} + gridText();
    return text;
}

int PianoRollPresenter::ticksPerQuarter() const
{
    return project().ticksPerQuarter();
}

std::int64_t PianoRollPresenter::ticksPerBar() const
{
    return project().ticksPerBar();
}

std::int64_t PianoRollPresenter::ticksPerBeat() const
{
    return engine::ticksPerBeat(ticksPerQuarter(), project().timeSignature().denominator);
}

std::int64_t PianoRollPresenter::lengthTicks() const
{
    const auto shown = pattern();
    return shown ? shown->lengthBars() * ticksPerBar() : 0;
}

// ----- Tools and grid -----------------------------------------------------------------------------

void PianoRollPresenter::setSnapEnabled(bool enabled)
{
    snapEnabled_ = enabled;
    ++changeCount_;
}

void PianoRollPresenter::setGrid(GridDivision division)
{
    grid_ = division;
    ++changeCount_;
}

std::int64_t PianoRollPresenter::gridStepTicks() const
{
    return ui::gridStepTicks(grid_, ticksPerQuarter());
}

// ----- Notes and pointer --------------------------------------------------------------------------

NoteHit PianoRollPresenter::hitAt(GridPoint point, std::int64_t tolerance) const
{
    return hit(point, tolerance).part;
}

void PianoRollPresenter::mouseDown(GridPoint point,
                                   PointerButton button,
                                   PointerModifiers modifiers,
                                   std::int64_t tolerance)
{
    drag_ = Drag::none;
    downPoint_ = point;
    dragPoint_ = point;
    const auto found = hit(point, tolerance);
    if (button == PointerButton::right || tool_ == PianoRollTool::erase)
    {
        undoManager_.beginNewTransaction(kDeleteNotes);
        drag_ = button == PointerButton::right ? Drag::none : Drag::erase;
        eraseAt(point);
        return;
    }
    pasteTick_ = std::clamp<std::int64_t>(placed(point.tick, modifiers, false), 0, lengthTicks());
    if (found.note != nullptr)
        grab(found, point, modifiers);
    else if (tool_ == PianoRollTool::draw)
        drawNote(point, modifiers);
    else
        startRectangle(point);
}

void PianoRollPresenter::mouseDrag(GridPoint point, PointerModifiers modifiers)
{
    dragPoint_ = point;
    if (drag_ == Drag::move)
        updateMove(point, modifiers);
    else if (drag_ == Drag::resize)
        updateResize(point, modifiers);
    else if (drag_ == Drag::erase)
        eraseAt(point);
    else if (drag_ == Drag::rectangle)
        ++changeCount_;
}

void PianoRollPresenter::mouseUp(GridPoint point, PointerModifiers modifiers)
{
    mouseDrag(point, modifiers);
    if (drag_ == Drag::move)
        commitMove();
    else if (drag_ == Drag::rectangle)
        finishRectangle(modifiers);
    drag_ = Drag::none;
    ++changeCount_;
}

std::vector<NoteGhost> PianoRollPresenter::ghosts() const
{
    std::vector<NoteGhost> result;
    if (drag_ != Drag::move || (moveTicks_ == 0 && moveRows_ == 0))
        return result;
    for (const auto& note : notes_)
        if (note.selected)
            result.push_back({note.row + moveRows_, note.start + moveTicks_, note.length});
    return result;
}

std::string PianoRollPresenter::dragTooltip() const
{
    const auto* grabbed = viewOf(grabbed_);
    if (drag_ != Drag::move || grabbed == nullptr || (moveTicks_ == 0 && moveRows_ == 0))
        return {};
    return "\xe2\x86\x92 " +
           formatPosition(grabbed->start + moveTicks_, ticksPerQuarter(), project().timeSignature());
}

std::optional<SelectionRect> PianoRollPresenter::selectionRect() const
{
    if (drag_ != Drag::rectangle)
        return std::nullopt;
    return SelectionRect{std::min(downPoint_.tick, dragPoint_.tick),
                         std::max(downPoint_.tick, dragPoint_.tick),
                         std::min(downPoint_.row, dragPoint_.row),
                         std::max(downPoint_.row, dragPoint_.row)};
}

// ----- Selection and clipboard --------------------------------------------------------------------

void PianoRollPresenter::selectAll()
{
    selection_.clear();
    for (const auto& note : notes_)
        selection_.push_back(note.tree);
    selectionChanged();
}

void PianoRollPresenter::deleteSelection()
{
    if (selection_.empty())
        return;
    undoManager_.beginNewTransaction(kDeleteNotes);
    removeNotes(selection_);
}

void PianoRollPresenter::copy()
{
    if (selection_.empty())
        return;
    clipboard_ = selectedData();
    const std::int64_t first = std::ranges::min(clipboard_, {}, &model::NoteData::startTick).startTick;
    std::int64_t end = 0;
    for (auto& note : clipboard_)
    {
        note.startTick -= first;
        end = std::max(end, note.startTick + note.lengthTicks);
    }
    const std::int64_t step = gridStepTicks();
    clipboardSpan_ = engine::floorDiv(end + step - 1, step) * step;
}

void PianoRollPresenter::cut()
{
    copy();
    if (selection_.empty())
        return;
    undoManager_.beginNewTransaction("Ausschneiden");
    removeNotes(selection_);
}

void PianoRollPresenter::paste()
{
    if (clipboard_.empty() || !hasPattern())
        return;
    undoManager_.beginNewTransaction(juce::String::fromUTF8("Einf\xc3\xbcgen"));
    selection_ = addNotes(clipboard_, pasteTick_);
    pasteTick_ += clipboardSpan_;
    selectionChanged();
}

void PianoRollPresenter::duplicate()
{
    if (selection_.empty())
        return;
    const auto data = selectedData();
    std::int64_t first = data.front().startTick;
    std::int64_t end = 0;
    for (const auto& note : data)
    {
        first = std::min(first, note.startTick);
        end = std::max(end, note.startTick + note.lengthTicks);
    }
    const std::int64_t step = gridStepTicks();
    undoManager_.beginNewTransaction("Duplizieren");
    selection_ = addNotes(data, engine::floorDiv(end - first + step - 1, step) * step);
    selectionChanged();
}

// ----- Velocity lane ------------------------------------------------------------------------------

std::optional<int> PianoRollPresenter::laneRow() const
{
    return laneGmNote_ ? rowOf(*laneGmNote_) : std::nullopt;
}

void PianoRollPresenter::setLaneRow(int row)
{
    if (row < 0 || row >= numRows())
        return;
    laneGmNote_ = rows_[static_cast<std::size_t>(row)].gmNote;
    ++changeCount_;
}

std::string PianoRollPresenter::laneLabel() const
{
    const auto row = laneRow();
    if (!row)
        return "Lane: Zeile w\xc3\xa4hlen";
    const auto& shown = rows_[static_cast<std::size_t>(*row)];
    return "Lane: " + shown.name + " (" + std::to_string(shown.midiNote) + ")";
}

void PianoRollPresenter::velocityDragBegin(std::int64_t tick, int velocity, std::int64_t tolerance)
{
    undoManager_.beginNewTransaction(kVelocityEdit);
    laneTick_ = tick;
    laneVelocity_ = velocity;
    laneTolerance_ = tolerance;
    applyVelocityLine(tick, velocity, tick, velocity);
}

void PianoRollPresenter::velocityDrag(std::int64_t tick, int velocity)
{
    applyVelocityLine(laneTick_, laneVelocity_, tick, velocity);
    laneTick_ = tick;
    laneVelocity_ = velocity;
}

int PianoRollPresenter::velocityAt(double y, double laneHeight)
{
    const double fraction = 1.0 - y / laneHeight;
    return std::clamp(static_cast<int>(std::lround(fraction * kMaxVelocity)), 1, kMaxVelocity);
}

double PianoRollPresenter::yOfVelocity(int velocity, double laneHeight)
{
    return laneHeight * (1.0 - static_cast<double>(velocity) / kMaxVelocity);
}

// ----- Inspector and status bar -------------------------------------------------------------------

std::optional<NoteDetails> PianoRollPresenter::noteDetails() const
{
    if (selection_.empty())
        return std::nullopt;
    const auto* first = viewOf(selection_.front());
    NoteDetails details;
    details.velocity = first->velocity;
    if (selection_.size() > 1)
    {
        details.instrument = plural(numSelected(), " Note", " Noten");
        details.position = kDash;
        details.length = kDash;
        return details;
    }
    const auto& row = rows_[static_cast<std::size_t>(first->row)];
    const int ppq = ticksPerQuarter();
    details.instrument = std::to_string(row.midiNote) + kSeparator + row.name;
    details.position = formatPosition(first->start, ppq, project().timeSignature());
    details.position += kSeparator + std::to_string(first->start) + " Ticks";
    if (const auto value = noteValueLabel(first->length, ppq))
        details.length = *value + kSeparator;
    details.length += std::to_string(first->length) + " Ticks";
    return details;
}

void PianoRollPresenter::setSelectedVelocity(int velocity)
{
    if (selection_.empty())
        return;
    if (!velocityEditing_)
        undoManager_.beginNewTransaction(kVelocityEdit);
    else if (undoManager_.getCurrentTransactionName() != kVelocityEdit) // something else was changed
        undoManager_.beginNewTransaction(kVelocityEdit);
    velocityEditing_ = true;
    for (const auto& tree : std::vector{selection_})
        model::Note{tree, &undoManager_}.setVelocity(velocity);
}

std::string PianoRollPresenter::statusText() const
{
    std::string text = "Snap: " + gridText();
    if (!selection_.empty())
        text += kSeparator + plural(numSelected(), " Note", " Noten") + " ausgew\xc3\xa4hlt";
    return text;
}

// ----- Private ------------------------------------------------------------------------------------

std::string PianoRollPresenter::gridText() const
{
    if (!snapEnabled_)
        return "aus";
    return gridLabel(grid_);
}

model::Project PianoRollPresenter::project() const
{
    return {project_, &undoManager_};
}

std::optional<model::Pattern> PianoRollPresenter::pattern() const
{
    const int index = active_.index();
    if (index < 0)
        return std::nullopt;
    return project().pattern(index);
}

void PianoRollPresenter::refresh()
{
    const auto shown = pattern();
    const std::string id = shown ? shown->id() : std::string{};
    if (id != shownPatternId_)
    {
        shownPatternId_ = id;
        selection_.clear();
        drag_ = Drag::none;
        pasteTick_ = 0;
    }
    rebuildRows(shown ? &*shown : nullptr);
    rebuildNotes(shown ? &*shown : nullptr);
    std::erase_if(selection_, [this](const juce::ValueTree& tree) { return viewOf(tree) == nullptr; });
    selectionChanged();
}

void PianoRollPresenter::rebuildRows(const model::Pattern* shown)
{
    std::array<bool, 128> used{};
    for (int index = 0; shown != nullptr && index < shown->numNotes(); ++index)
        used.at(static_cast<std::size_t>(std::clamp(shown->note(index).slotNote(), 0, 127))) = true;

    const auto kit = project().activeKit(globalKit_);
    std::vector<std::pair<PianoRollRow, int>> rows;
    for (int slot = 0; slot < kit.numSlots(); ++slot)
    {
        const auto sampleSlot = kit.slot(slot);
        const int gmNote = sampleSlot.gmNote();
        if (engine::isCoreGmNote(gmNote) || used.at(static_cast<std::size_t>(std::clamp(gmNote, 0, 127))))
            rows.push_back({{gmNote, sampleSlot.midiNote(), sampleSlot.name()}, slot});
    }
    std::ranges::stable_sort(rows, std::greater{}, [](const auto& row) { return row.first.midiNote; });
    rows_.clear();
    rowSlots_.clear();
    for (auto& [row, slot] : rows)
    {
        rows_.push_back(std::move(row));
        rowSlots_.push_back(slot);
    }
}

void PianoRollPresenter::rebuildNotes(const model::Pattern* shown)
{
    notes_.clear();
    if (shown == nullptr)
        return;
    const std::int64_t length = lengthTicks();
    for (int index = 0; index < shown->numNotes(); ++index)
    {
        const auto note = shown->note(index);
        const auto data = note.data();
        const auto row = rowOf(data.slotNote);
        if (row && data.startTick < length)
            notes_.push_back(
                {note.tree(), *row, data.startTick, data.lengthTicks, data.velocity, data.origin, false});
    }
}

void PianoRollPresenter::selectionChanged()
{
    for (auto& note : notes_)
        note.selected = isSelected(note.tree);
    ++changeCount_;
}

std::optional<int> PianoRollPresenter::rowOf(int gmNote) const
{
    const auto found = std::ranges::find(rows_, gmNote, &PianoRollRow::gmNote);
    if (found == rows_.end())
        return std::nullopt;
    return static_cast<int>(found - rows_.begin());
}

const NoteView* PianoRollPresenter::viewOf(const juce::ValueTree& tree) const
{
    const auto found = std::ranges::find(notes_, tree, &NoteView::tree);
    return found == notes_.end() ? nullptr : &*found;
}

bool PianoRollPresenter::isSelected(const juce::ValueTree& tree) const
{
    return std::ranges::find(selection_, tree) != selection_.end();
}

PianoRollPresenter::Hit PianoRollPresenter::hit(GridPoint point, std::int64_t tolerance) const
{
    // Later notes are drawn on top, so they are hit first.
    for (auto note = notes_.rbegin(); note != notes_.rend(); ++note)
    {
        const std::int64_t end = note->start + note->length;
        if (note->row != point.row || point.tick < note->start || point.tick >= end + tolerance)
            continue;
        const std::int64_t endZone = std::min(tolerance, note->length / 2);
        return {&*note, point.tick >= end - endZone ? NoteHit::end : NoteHit::body};
    }
    return {};
}

std::int64_t PianoRollPresenter::placed(std::int64_t tick, PointerModifiers modifiers, bool nearest) const
{
    if (!snapEnabled_ || modifiers.alt)
        return tick;
    return nearest ? snapToGrid(tick, gridStepTicks()) : floorToGrid(tick, gridStepTicks());
}

void PianoRollPresenter::grab(const Hit& found, GridPoint point, PointerModifiers modifiers)
{
    const auto tree = found.note->tree;
    setLaneRow(found.note->row);
    if (modifiers.ctrl && isSelected(tree))
    {
        std::erase(selection_, tree);
        selectionChanged();
        return;
    }
    if (modifiers.ctrl)
        selection_.push_back(tree);
    else if (!isSelected(tree))
        selection_.assign(1, tree);
    selectionChanged();
    grabbed_ = tree;
    grabOffset_ = point.tick - found.note->start;
    moveTicks_ = 0;
    moveRows_ = 0;
    drag_ = found.part == NoteHit::end ? Drag::resize : Drag::move;
    if (drag_ == Drag::resize)
        undoManager_.beginNewTransaction(juce::String::fromUTF8("Notenl\xc3\xa4nge"));
}

void PianoRollPresenter::drawNote(GridPoint point, PointerModifiers modifiers)
{
    const std::int64_t start = placed(point.tick, modifiers, false);
    const std::int64_t length = lengthTicks();
    if (point.row < 0 || point.row >= numRows() || start < 0 || start >= length)
        return;
    undoManager_.beginNewTransaction("Note zeichnen");
    auto note = pattern()->addNote({.slotNote = rows_[static_cast<std::size_t>(point.row)].gmNote,
                                    .startTick = start,
                                    .lengthTicks = std::min(gridStepTicks(), length - start),
                                    .velocity = kDefaultVelocity,
                                    .origin = model::NoteOrigin::grid});
    selection_.assign(1, note.tree());
    setLaneRow(point.row);
    selectionChanged();
    grabbed_ = note.tree();
    drag_ = Drag::resize;
}

void PianoRollPresenter::startRectangle(GridPoint point)
{
    downPoint_ = point;
    drag_ = Drag::rectangle;
}

void PianoRollPresenter::eraseAt(GridPoint point)
{
    const auto found = hit(point, 0);
    if (found.note == nullptr)
        return;
    const juce::ValueTree tree = found.note->tree;
    removeNote(tree);
}

void PianoRollPresenter::updateMove(GridPoint point, PointerModifiers modifiers)
{
    const auto* grabbed = viewOf(grabbed_);
    if (grabbed == nullptr)
        return;
    moveIsFree_ = !snapEnabled_ || modifiers.alt;
    std::int64_t ticks = placed(point.tick - grabOffset_, modifiers, true) - grabbed->start;
    int rows = point.row - downPoint_.row;
    // A click without movement only selects; it must not snap a played-in note.
    if (point.tick == downPoint_.tick && rows == 0)
        ticks = 0;
    for (const auto& note : notes_)
        if (note.selected)
        {
            ticks = std::clamp(
                ticks, -note.start, std::max(-note.start, lengthTicks() - note.start - note.length));
            rows = std::clamp(rows, -note.row, numRows() - 1 - note.row);
        }
    moveTicks_ = ticks;
    moveRows_ = rows;
    ++changeCount_;
}

void PianoRollPresenter::updateResize(GridPoint point, PointerModifiers modifiers)
{
    const auto* resized = viewOf(grabbed_);
    // Without movement a click keeps the length, also of a note just drawn.
    if (resized == nullptr || point.tick == downPoint_.tick)
        return;
    const bool free = !snapEnabled_ || modifiers.alt;
    const std::int64_t minimum = free ? 1 : gridStepTicks();
    const std::int64_t end = placed(point.tick, modifiers, true);
    const std::int64_t length = std::clamp(end - resized->start,
                                           std::min(minimum, lengthTicks() - resized->start),
                                           lengthTicks() - resized->start);
    if (length != resized->length)
        model::Note{grabbed_, &undoManager_}.setLengthTicks(length);
}

void PianoRollPresenter::commitMove()
{
    if (moveTicks_ == 0 && moveRows_ == 0)
        return;
    std::vector<NoteView> moved;
    std::ranges::copy_if(notes_, std::back_inserter(moved), &NoteView::selected);
    const std::int64_t step = gridStepTicks();
    undoManager_.beginNewTransaction("Noten verschieben");
    for (const auto& view : moved)
    {
        model::Note note{view.tree, &undoManager_};
        const std::int64_t start = view.start + moveTicks_;
        note.setStartTick(start);
        note.setSlotNote(rows_[static_cast<std::size_t>(view.row + moveRows_)].gmNote);
        // Snapping a played-in note onto the grid makes it a gridded note (F-PR-07).
        if (!moveIsFree_ && start % step == 0)
            note.setOrigin(model::NoteOrigin::grid);
    }
    moveTicks_ = 0;
    moveRows_ = 0;
}

void PianoRollPresenter::finishRectangle(PointerModifiers modifiers)
{
    const auto rect = *selectionRect();
    if (!modifiers.ctrl)
        selection_.clear();
    for (const auto& note : notes_)
        if (note.row >= rect.firstRow && note.row <= rect.lastRow && note.start <= rect.endTick &&
            note.start + note.length > rect.startTick && !isSelected(note.tree))
            selection_.push_back(note.tree);
    selectionChanged();
}

void PianoRollPresenter::removeNotes(const std::vector<juce::ValueTree>& trees)
{
    for (const auto& tree : std::vector{trees})
        removeNote(tree);
}

void PianoRollPresenter::removeNote(const juce::ValueTree& tree)
{
    auto parent = tree.getParent();
    parent.removeChild(tree, &undoManager_);
}

std::vector<juce::ValueTree> PianoRollPresenter::addNotes(const std::vector<model::NoteData>& notes,
                                                          std::int64_t offset)
{
    auto shown = *pattern();
    const std::int64_t length = lengthTicks();
    std::vector<juce::ValueTree> added;
    for (auto note : notes)
    {
        note.startTick += offset;
        if (note.startTick < length)
            added.push_back(shown.addNote(note).tree());
    }
    return added;
}

std::vector<model::NoteData> PianoRollPresenter::selectedData() const
{
    std::vector<model::NoteData> data;
    for (const auto& tree : selection_)
        data.push_back(model::Note{tree, nullptr}.data());
    return data;
}

void PianoRollPresenter::applyVelocityLine(std::int64_t fromTick,
                                           int fromVelocity,
                                           std::int64_t toTick,
                                           int toVelocity)
{
    const auto row = laneRow();
    if (!row)
        return;
    const std::int64_t low = std::min(fromTick, toTick) - laneTolerance_;
    const std::int64_t high = std::max(fromTick, toTick) + laneTolerance_;
    std::vector<std::pair<juce::ValueTree, int>> changes;
    for (const auto& note : notes_)
    {
        if (note.row != *row || note.start < low || note.start > high)
            continue;
        const double fraction = fractionOf(note.start, fromTick, toTick);
        changes.emplace_back(
            note.tree, static_cast<int>(std::lround(fromVelocity + fraction * (toVelocity - fromVelocity))));
    }
    for (const auto& [tree, velocity] : changes)
        model::Note{tree, &undoManager_}.setVelocity(velocity);
}

} // namespace drumprog::ui
