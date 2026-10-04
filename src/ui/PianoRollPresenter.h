#pragma once

#include "model/Project.h"
#include "model/TreeChangeListener.h"
#include "ui/ActivePattern.h"
#include "ui/SnapGrid.h"

#include <juce_data_structures/juce_data_structures.h>

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::ui
{

enum class PianoRollTool : std::uint8_t
{
    draw,
    select,
    erase
};

enum class PointerButton : std::uint8_t
{
    left,
    right
};

/// Part of a note under the pointer: dragging the body moves it, dragging the end changes its length.
enum class NoteHit : std::uint8_t
{
    none,
    body,
    end
};

/// A point of the note area in musical units; the view converts pixels with PianoRollGeometry.
struct GridPoint
{
    std::int64_t tick = 0;
    int row = 0;
};

struct PointerModifiers
{
    bool ctrl = false; ///< adds to or removes from the selection (F-PR-05)
    bool alt = false;  ///< places freely, without snap (F-PR-04)
};

/// One row of the piano roll: a slot of the active kit (F-PR-01).
struct PianoRollRow
{
    int gmNote = 0;
    int midiNote = 0;
    std::string name;
};

struct NoteView
{
    juce::ValueTree tree;
    int row = 0;
    std::int64_t start = 0;
    std::int64_t length = 0;
    int velocity = 0;
    model::NoteOrigin origin = model::NoteOrigin::grid;
    bool selected = false;
};

/// Frame at the target of a note being moved (F-PR-06).
struct NoteGhost
{
    int row = 0;
    std::int64_t start = 0;
    std::int64_t length = 0;
};

/// Rectangle selection being dragged, in ticks and rows, both ends included.
struct SelectionRect
{
    std::int64_t startTick = 0;
    std::int64_t endTick = 0;
    int firstRow = 0;
    int lastRow = 0;
};

/// The note part of the inspector.
struct NoteDetails
{
    std::string instrument; ///< "38 · Acoustic Snare", or "3 Noten"
    std::string position;   ///< "002.2.000 · 4800 Ticks"
    std::string length;     ///< "1/16 · 240 Ticks"
    int velocity = 0;
};

/// Logic of the piano roll (F-PR-01 to 10, humble view app/PianoRollView), its velocity lane and the
/// note part of the inspector. It shows and edits the active pattern; rows are the core slots of the
/// active kit plus every slot the pattern has notes for, sorted by MIDI note, highest first. Every
/// edit is one undo step (F-PJ-05), and every change of the model, recordings included (F-IN-10),
/// shows at once. GUI thread only.
class PianoRollPresenter
{
public:
    static constexpr int kDefaultVelocity = 100;

    PianoRollPresenter(juce::ValueTree project,
                       juce::ValueTree globalKit,
                       juce::UndoManager& undoManager,
                       ActivePattern& active,
                       std::function<std::string(int slotIndex)> keyLabel);

    // ----- Rows and pattern -----
    [[nodiscard]] const std::vector<PianoRollRow>& rows() const noexcept { return rows_; }
    [[nodiscard]] int numRows() const noexcept { return static_cast<int>(rows_.size()); }
    /// Key that triggers the row's slot, for the badge (F-PR-01).
    [[nodiscard]] std::string rowKey(int row) const;
    [[nodiscard]] bool hasPattern() const;
    /// "Piano-Roll — Pattern 1 · 2 Takte · 4/4 · Raster 1/16".
    [[nodiscard]] std::string headerText() const;
    [[nodiscard]] int ticksPerQuarter() const;
    [[nodiscard]] std::int64_t ticksPerBar() const;
    [[nodiscard]] std::int64_t ticksPerBeat() const;
    [[nodiscard]] std::int64_t lengthTicks() const;

    // ----- Tools and grid (F-PR-02, F-PR-04) -----
    void setTool(PianoRollTool tool) noexcept { tool_ = tool; }
    [[nodiscard]] PianoRollTool tool() const noexcept { return tool_; }
    void setSnapEnabled(bool enabled);
    [[nodiscard]] bool snapEnabled() const noexcept { return snapEnabled_; }
    void setGrid(GridDivision division);
    [[nodiscard]] GridDivision grid() const noexcept { return grid_; }
    [[nodiscard]] std::int64_t gridStepTicks() const;

    // ----- Notes and pointer (F-PR-03, F-PR-05, F-PR-06) -----
    /// Notes of the active pattern inside its length, in drawing order.
    [[nodiscard]] const std::vector<NoteView>& notes() const noexcept { return notes_; }
    /// tolerance: ticks around a note's end that grab the end.
    [[nodiscard]] NoteHit hitAt(GridPoint point, std::int64_t tolerance) const;
    void mouseDown(GridPoint point, PointerButton button, PointerModifiers modifiers, std::int64_t tolerance);
    void mouseDrag(GridPoint point, PointerModifiers modifiers);
    void mouseUp(GridPoint point, PointerModifiers modifiers);
    /// Targets of the notes being moved; empty unless a move is dragged.
    [[nodiscard]] std::vector<NoteGhost> ghosts() const;
    /// "→ 002.1.240" while moving, otherwise empty.
    [[nodiscard]] std::string dragTooltip() const;
    [[nodiscard]] std::optional<SelectionRect> selectionRect() const;

    // ----- Selection and clipboard (F-PR-05) -----
    [[nodiscard]] int numSelected() const noexcept { return static_cast<int>(selection_.size()); }
    void selectAll();
    void deleteSelection();
    void copy();
    void cut();
    /// Inserts the copied notes at the last clicked position; repeated pastes follow each other.
    void paste();
    [[nodiscard]] bool canPaste() const noexcept { return !clipboard_.empty(); }
    /// Copies the selection right behind itself, rounded up to the grid, and selects the copies.
    void duplicate();

    // ----- Velocity lane (F-PR-10) -----
    /// Row whose notes the lane shows; the last clicked note or row header chooses it.
    [[nodiscard]] std::optional<int> laneRow() const;
    void setLaneRow(int row);
    /// "Lane: Closed Hi-Hat (42)".
    [[nodiscard]] std::string laneLabel() const;
    /// Every note of the lane row whose start lies between the previous and the current pointer
    /// position gets the velocity of the line between them, so one stroke sets several notes.
    void velocityDragBegin(std::int64_t tick, int velocity, std::int64_t tolerance);
    void velocityDrag(std::int64_t tick, int velocity);
    [[nodiscard]] static int velocityAt(double y, double laneHeight);
    [[nodiscard]] static double yOfVelocity(int velocity, double laneHeight);

    // ----- Inspector and status bar -----
    [[nodiscard]] std::optional<NoteDetails> noteDetails() const;
    /// Sets the velocity of all selected notes; changes until endVelocityEdit() form one undo step.
    void setSelectedVelocity(int velocity);
    void endVelocityEdit() noexcept { velocityEditing_ = false; }
    /// "Snap: 1/16 · 2 Noten ausgewählt".
    [[nodiscard]] std::string statusText() const;

    /// Changes whenever something shown changed: the model, the active pattern or the selection.
    [[nodiscard]] std::uint32_t changeCount() const noexcept { return changeCount_; }

private:
    enum class Drag : std::uint8_t
    {
        none,
        move,
        resize,
        erase,
        rectangle
    };

    struct Hit
    {
        const NoteView* note = nullptr;
        NoteHit part = NoteHit::none;
    };

    [[nodiscard]] std::string gridText() const;
    [[nodiscard]] model::Project project() const;
    [[nodiscard]] std::optional<model::Pattern> pattern() const;
    void refresh();
    void rebuildRows(const model::Pattern* shown);
    void rebuildNotes(const model::Pattern* shown);
    void selectionChanged();
    [[nodiscard]] std::optional<int> rowOf(int gmNote) const;
    [[nodiscard]] const NoteView* viewOf(const juce::ValueTree& tree) const;
    [[nodiscard]] bool isSelected(const juce::ValueTree& tree) const;
    [[nodiscard]] Hit hit(GridPoint point, std::int64_t tolerance) const;
    [[nodiscard]] std::int64_t placed(std::int64_t tick, PointerModifiers modifiers, bool nearest) const;

    void grab(const Hit& found, GridPoint point, PointerModifiers modifiers);
    void drawNote(GridPoint point, PointerModifiers modifiers);
    void startRectangle(GridPoint point);
    void eraseAt(GridPoint point);
    void updateMove(GridPoint point, PointerModifiers modifiers);
    void updateResize(GridPoint point, PointerModifiers modifiers);
    void commitMove();
    void finishRectangle(const SelectionRect& rect, PointerModifiers modifiers);
    void removeNotes(const std::vector<juce::ValueTree>& trees);
    void removeNote(const juce::ValueTree& tree);
    std::vector<juce::ValueTree>
    addNotes(model::Pattern& shown, const std::vector<model::NoteData>& notes, std::int64_t offset) const;
    [[nodiscard]] std::vector<model::NoteData> selectedData() const;
    void applyVelocityLine(std::int64_t fromTick, int fromVelocity, std::int64_t toTick, int toVelocity);

    juce::ValueTree project_;
    juce::ValueTree globalKit_;
    juce::UndoManager& undoManager_;
    ActivePattern& active_;
    std::function<std::string(int)> keyLabel_;

    PianoRollTool tool_ = PianoRollTool::draw;
    GridDivision grid_ = GridDivision::sixteenth;
    bool snapEnabled_ = true;

    std::vector<PianoRollRow> rows_;
    std::vector<int> rowSlots_; ///< kit slot index of each row
    std::vector<NoteView> notes_;
    std::vector<juce::ValueTree> selection_;
    std::string shownPatternId_;

    Drag drag_ = Drag::none;
    GridPoint downPoint_;
    GridPoint dragPoint_;
    juce::ValueTree grabbed_;
    std::int64_t grabOffset_ = 0;
    std::int64_t moveTicks_ = 0;
    int moveRows_ = 0;
    bool moveIsFree_ = false;

    std::vector<model::NoteData> clipboard_; ///< start ticks relative to the first copied note
    std::int64_t clipboardSpan_ = 0;
    std::int64_t pasteTick_ = 0;

    std::optional<int> laneGmNote_;
    std::int64_t laneTick_ = 0;
    int laneVelocity_ = 0;
    std::int64_t laneTolerance_ = 0;
    bool velocityEditing_ = false;

    std::uint32_t changeCount_ = 0;
    model::TreeChangeListener projectListener_;
    model::TreeChangeListener globalKitListener_;
};

} // namespace drumprog::ui
