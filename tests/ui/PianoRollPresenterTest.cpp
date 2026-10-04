#include "ui/PianoRollPresenter.h"

#include "TestComparisons.h"
#include "model/FakeIdGenerator.h"
#include "model/ModelIds.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>

namespace drumprog::ui
{
namespace
{

using model::NoteOrigin;

constexpr PointerModifiers kNone{};
constexpr PointerModifiers kCtrl{.ctrl = true};
constexpr PointerModifiers kAlt{.alt = true};
constexpr std::int64_t kLength = 7680; // two bars 4/4

class PianoRollPresenterTest : public ::testing::Test
{
protected:
    /// Row of the slot with this GM note.
    [[nodiscard]] int row(int gmNote) const
    {
        const auto& rows = presenter.rows();
        return static_cast<int>(std::ranges::find(rows, gmNote, &PianoRollRow::gmNote) - rows.begin());
    }

    [[nodiscard]] GridPoint at(std::int64_t tick, int gmNote) const { return {tick, row(gmNote)}; }

    [[nodiscard]] model::Pattern pattern() const { return project.pattern(0); }

    model::Note addNote(int gmNote,
                        std::int64_t start,
                        std::int64_t length = 240,
                        int velocity = 100,
                        NoteOrigin origin = NoteOrigin::grid)
    {
        return pattern().addNote({gmNote, start, length, velocity, origin});
    }

    void click(GridPoint point, PointerModifiers modifiers = kNone, std::int64_t tolerance = 0)
    {
        presenter.mouseDown(point, PointerButton::left, modifiers, tolerance);
        presenter.mouseUp(point, modifiers);
    }

    void drag(GridPoint from, GridPoint to, PointerModifiers modifiers = kNone, std::int64_t tolerance = 0)
    {
        presenter.mouseDown(from, PointerButton::left, modifiers, tolerance);
        presenter.mouseDrag(to, modifiers);
        presenter.mouseUp(to, modifiers);
    }

    void rightClick(GridPoint point)
    {
        presenter.mouseDown(point, PointerButton::right, kNone, 0);
        presenter.mouseUp(point, kNone);
    }

    [[nodiscard]] std::vector<std::int64_t> starts() const
    {
        std::vector<std::int64_t> result;
        for (int index = 0; index < pattern().numNotes(); ++index)
            result.push_back(pattern().note(index).startTick());
        std::ranges::sort(result);
        return result;
    }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    juce::ValueTree globalKit = model::ProjectFactory::createDefaultKit();
    model::Project project{tree, nullptr};
    juce::UndoManager undoManager;
    ActivePattern active{tree};
    PianoRollPresenter presenter{
        tree, globalKit, undoManager, active, [](int slot) { return "K" + std::to_string(slot); }};
};

// ----- Rows ---------------------------------------------------------------------------------------

TEST_F(PianoRollPresenterTest, FPR01_RowsAreTheCoreSlotsSortedByMidiNoteHighestFirst)
{
    ASSERT_EQ(presenter.numRows(), 19);
    EXPECT_EQ(presenter.rows().front().gmNote, 57);
    EXPECT_EQ(presenter.rows().back().gmNote, 36);
    EXPECT_EQ(presenter.rows().back().name, "Bass Drum 1");
    EXPECT_EQ(presenter.rows().back().midiNote, 36);
    EXPECT_EQ(presenter.rowKey(18), "K1");
    EXPECT_EQ(presenter.rowKey(-1), "");
    EXPECT_EQ(presenter.rowKey(19), "");
}

TEST_F(PianoRollPresenterTest, FPR01_SlotsWithNotesGetARowToo)
{
    addNote(54, 0);
    addNote(127, 0); // no such slot in the kit

    EXPECT_EQ(presenter.numRows(), 20);
    EXPECT_EQ(row(54), row(55) + 1);
    EXPECT_EQ(presenter.notes().size(), 1U);
}

TEST_F(PianoRollPresenterTest, FPR01_RowsFollowTheMidiNoteOfTheActiveKit)
{
    model::Kit{globalKit, nullptr}.findSlot(36)->setMidiNote(60);
    EXPECT_EQ(presenter.rows().front().gmNote, 36);

    project.setOwnKit(model::ProjectFactory::createDefaultKit());
    EXPECT_EQ(presenter.rows().front().gmNote, 57);
}

TEST_F(PianoRollPresenterTest, FPR09_HeaderShowsNameLengthTimeSignatureAndGrid)
{
    EXPECT_EQ(presenter.headerText(),
              "Piano-Roll \xe2\x80\x94 Pattern 1 \xc2\xb7 2 Takte \xc2\xb7 4/4 \xc2\xb7 Raster 1/16");

    pattern().setLengthBars(1);
    presenter.setSnapEnabled(false);
    EXPECT_EQ(presenter.headerText(),
              "Piano-Roll \xe2\x80\x94 Pattern 1 \xc2\xb7 1 Takt \xc2\xb7 4/4 \xc2\xb7 Raster aus");
}

TEST_F(PianoRollPresenterTest, FPR08_TimingOfTheGrid)
{
    EXPECT_TRUE(presenter.hasPattern());
    EXPECT_EQ(presenter.ticksPerQuarter(), 960);
    EXPECT_EQ(presenter.ticksPerBar(), 3840);
    EXPECT_EQ(presenter.lengthTicks(), kLength);
    EXPECT_EQ(presenter.gridStepTicks(), 240);

    project.setTimeSignature({6, 8});
    EXPECT_EQ(presenter.ticksPerBeat(), 480);
    presenter.setGrid(GridDivision::eighthTriplet);
    EXPECT_EQ(presenter.grid(), GridDivision::eighthTriplet);
    EXPECT_EQ(presenter.gridStepTicks(), 320);
}

TEST_F(PianoRollPresenterTest, FPR01_WithoutPatternNothingIsShownOrEdited)
{
    tree.getChildWithName(model::ids::patterns).removeAllChildren(nullptr);

    EXPECT_FALSE(presenter.hasPattern());
    EXPECT_EQ(presenter.headerText(), "Piano-Roll");
    EXPECT_EQ(presenter.lengthTicks(), 0);
    EXPECT_EQ(presenter.numRows(), 19);
    click({0, 0});
    EXPECT_TRUE(presenter.notes().empty());
}

// ----- Drawing ------------------------------------------------------------------------------------

TEST_F(PianoRollPresenterTest, FPR03_ClickDrawsAGriddedNoteOnTheGrid)
{
    EXPECT_EQ(presenter.tool(), PianoRollTool::draw);

    click(at(1000, 38));

    ASSERT_EQ(pattern().numNotes(), 1);
    EXPECT_EQ(pattern().note(0).data(), (model::NoteData{38, 960, 240, 100, NoteOrigin::grid}));
    EXPECT_EQ(presenter.numSelected(), 1);
    EXPECT_TRUE(presenter.notes().front().selected);

    undoManager.undo();
    EXPECT_EQ(pattern().numNotes(), 0);
    EXPECT_EQ(presenter.numSelected(), 0);
}

TEST_F(PianoRollPresenterTest, FPR03_DraggingAfterDrawingSetsTheLengthInOneStep)
{
    drag(at(1000, 38), at(1700, 38));

    EXPECT_EQ(pattern().note(0).lengthTicks(), 720);
    undoManager.undo();
    EXPECT_EQ(pattern().numNotes(), 0);
}

TEST_F(PianoRollPresenterTest, FPR04_AltDrawsFreely)
{
    drag(at(1000, 38), at(1100, 38), kAlt);
    EXPECT_EQ(pattern().note(0).startTick(), 1000);
    EXPECT_EQ(pattern().note(0).lengthTicks(), 100);

    presenter.setSnapEnabled(false);
    EXPECT_FALSE(presenter.snapEnabled());
    click(at(2001, 42));
    EXPECT_EQ(pattern().note(1).startTick(), 2001);
}

TEST_F(PianoRollPresenterTest, FPR03_NotesStayInsideThePattern)
{
    click(at(7600, 38), kAlt);
    EXPECT_EQ(pattern().note(0).lengthTicks(), 80);

    click({1000, -1});
    click({1000, 19});
    click(at(-5, 38), kAlt);
    click(at(kLength, 38));
    EXPECT_EQ(pattern().numNotes(), 1);
}

// ----- Moving and resizing ------------------------------------------------------------------------

TEST_F(PianoRollPresenterTest, FPR03_DragMovesANoteAlsoToAnotherRow)
{
    addNote(38, 960);

    presenter.mouseDown(at(1000, 38), PointerButton::left, kNone, 0);
    presenter.mouseDrag(at(1500, 42), kNone);

    const auto ghosts = presenter.ghosts();
    ASSERT_EQ(ghosts.size(), 1U);
    EXPECT_EQ(ghosts[0].start, 1440);
    EXPECT_EQ(ghosts[0].row, row(42));
    EXPECT_EQ(ghosts[0].length, 240);
    EXPECT_EQ(presenter.dragTooltip(), "\xe2\x86\x92 001.2.480");
    EXPECT_EQ(pattern().note(0).startTick(), 960); // unchanged until the button is released

    presenter.mouseUp(at(1500, 42), kNone);
    EXPECT_EQ(pattern().note(0).startTick(), 1440);
    EXPECT_EQ(pattern().note(0).slotNote(), 42);
    EXPECT_TRUE(presenter.ghosts().empty());
    EXPECT_EQ(presenter.dragTooltip(), "");

    undoManager.undo();
    EXPECT_EQ(pattern().note(0).startTick(), 960);
    EXPECT_EQ(pattern().note(0).slotNote(), 38);
}

TEST_F(PianoRollPresenterTest, FPR06_GhostsShowOnlyTheSelectedNotesAlsoForRowChanges)
{
    addNote(38, 960);
    addNote(36, 960);

    presenter.mouseDown(at(1000, 38), PointerButton::left, kNone, 0);
    presenter.mouseDrag(at(1000, 42), kNone);

    const auto ghosts = presenter.ghosts();
    ASSERT_EQ(ghosts.size(), 1U);
    EXPECT_EQ(ghosts[0].start, 960);
    EXPECT_EQ(ghosts[0].row, row(42));
    EXPECT_EQ(presenter.dragTooltip(), "\xe2\x86\x92 001.2.000");
    presenter.mouseUp(at(1000, 42), kNone);
    EXPECT_EQ(pattern().note(0).slotNote(), 42);
    EXPECT_EQ(pattern().note(1).slotNote(), 36);
}

TEST_F(PianoRollPresenterTest, FPR04_WithoutSnapNotesMoveAndResizeFreely)
{
    addNote(38, 960);
    presenter.setSnapEnabled(false);

    drag(at(1000, 38), at(1013, 38));
    EXPECT_EQ(pattern().note(0).startTick(), 973);

    drag(at(1210, 38), at(1250, 38), kNone, 20);
    EXPECT_EQ(pattern().note(0).lengthTicks(), 277);
}

TEST_F(PianoRollPresenterTest, FPR06_NoGhostWithoutMovement)
{
    addNote(38, 960);
    presenter.mouseDown(at(1000, 38), PointerButton::left, kNone, 0);
    presenter.mouseDrag(at(1010, 38), kNone);

    EXPECT_TRUE(presenter.ghosts().empty());
    EXPECT_EQ(presenter.dragTooltip(), "");
    presenter.mouseUp(at(1010, 38), kNone);
    EXPECT_EQ(undoManager.getNumActionsInCurrentTransaction(), 0);
}

TEST_F(PianoRollPresenterTest, FPR03_MovedNotesStayInsideThePatternAndRows)
{
    addNote(38, 960);

    drag(at(1000, 38), {-5000, -4});
    EXPECT_EQ(pattern().note(0).startTick(), 0);
    EXPECT_EQ(pattern().note(0).slotNote(), 57);

    drag(at(10, 57), {20000, 40});
    EXPECT_EQ(pattern().note(0).startTick(), kLength - 240);
    EXPECT_EQ(pattern().note(0).slotNote(), 36);
}

TEST_F(PianoRollPresenterTest, FPR07_SnappingAPlayedInNoteMakesItGridded)
{
    addNote(38, 1000, 240, 100, NoteOrigin::live);

    drag(at(1010, 38), at(1500, 38));

    EXPECT_EQ(pattern().note(0).startTick(), 1440);
    EXPECT_EQ(pattern().note(0).origin(), NoteOrigin::grid);
}

TEST_F(PianoRollPresenterTest, FPR04_AltMovesFreelyAndKeepsTheOrigin)
{
    addNote(38, 1000, 240, 100, NoteOrigin::live);

    drag(at(1010, 38), at(1515, 38), kAlt);

    EXPECT_EQ(pattern().note(0).startTick(), 1505);
    EXPECT_EQ(pattern().note(0).origin(), NoteOrigin::live);
}

TEST_F(PianoRollPresenterTest, FPR07_NotesMovedOffTheGridKeepTheirOrigin)
{
    addNote(38, 960);
    addNote(42, 1000, 240, 100, NoteOrigin::live);
    presenter.selectAll();

    drag(at(970, 38), at(1210, 38));

    EXPECT_EQ(pattern().note(0).startTick(), 1200);
    EXPECT_EQ(pattern().note(1).startTick(), 1240);
    EXPECT_EQ(pattern().note(1).origin(), NoteOrigin::live);
}

TEST_F(PianoRollPresenterTest, FPR03_ClickOnAPlayedInNoteDoesNotMoveIt)
{
    addNote(38, 1000, 240, 100, NoteOrigin::live);

    click(at(1010, 38));

    EXPECT_EQ(pattern().note(0).startTick(), 1000);
    EXPECT_EQ(pattern().note(0).origin(), NoteOrigin::live);
    EXPECT_EQ(presenter.numSelected(), 1);
}

TEST_F(PianoRollPresenterTest, FPR03_TheEndOfANoteIsGrabbedToChangeItsLength)
{
    addNote(38, 960);
    EXPECT_EQ(presenter.hitAt(at(500, 38), 30), NoteHit::none);
    EXPECT_EQ(presenter.hitAt(at(1000, 38), 30), NoteHit::body);
    EXPECT_EQ(presenter.hitAt(at(1190, 38), 30), NoteHit::end);
    EXPECT_EQ(presenter.hitAt(at(1220, 38), 30), NoteHit::end);
    EXPECT_EQ(presenter.hitAt(at(1000, 42), 30), NoteHit::none);

    presenter.mouseDown(at(1190, 38), PointerButton::left, kNone, 30);
    presenter.mouseDrag(at(1500, 38), kNone);
    EXPECT_EQ(pattern().note(0).lengthTicks(), 480);
    presenter.mouseDrag(at(900, 38), kNone);
    EXPECT_EQ(pattern().note(0).lengthTicks(), 240);
    presenter.mouseDrag(at(1300, 38), kAlt);
    presenter.mouseUp(at(1300, 38), kAlt);
    EXPECT_EQ(pattern().note(0).lengthTicks(), 340);

    undoManager.undo();
    EXPECT_EQ(pattern().note(0).lengthTicks(), 240);
}

TEST_F(PianoRollPresenterTest, FPR03_LengthEndsAtThePatternEnd)
{
    addNote(38, 7600, 40);
    drag(at(7630, 38), at(9000, 38), kNone, 30);
    EXPECT_EQ(pattern().note(0).lengthTicks(), 80);
}

TEST_F(PianoRollPresenterTest, FPR03_ANoteRemovedWhileDraggingEndsTheDrag)
{
    addNote(38, 960);
    presenter.mouseDown(at(1000, 38), PointerButton::left, kNone, 0);
    pattern().removeNote(0);
    presenter.mouseDrag(at(1500, 38), kNone);
    EXPECT_EQ(presenter.dragTooltip(), "");
    presenter.mouseUp(at(1500, 38), kNone);

    addNote(38, 960);
    presenter.mouseDown(at(1190, 38), PointerButton::left, kNone, 30);
    pattern().removeNote(0);
    presenter.mouseUp(at(1500, 38), kNone);
    EXPECT_EQ(pattern().numNotes(), 0);
}

// ----- Deleting -----------------------------------------------------------------------------------

TEST_F(PianoRollPresenterTest, FPR03_RightClickDeletesANote)
{
    addNote(38, 960);

    rightClick(at(500, 38));
    EXPECT_EQ(pattern().numNotes(), 1);
    rightClick(at(1000, 38));
    EXPECT_EQ(pattern().numNotes(), 0);

    undoManager.undo();
    EXPECT_EQ(pattern().numNotes(), 1);
}

TEST_F(PianoRollPresenterTest, FPR02_EraserDeletesEveryNoteItIsDraggedOver)
{
    addNote(38, 960);
    addNote(38, 1440);
    presenter.setTool(PianoRollTool::erase);

    drag(at(1000, 38), at(1500, 38));

    EXPECT_EQ(pattern().numNotes(), 0);
    undoManager.undo();
    EXPECT_EQ(pattern().numNotes(), 2);
}

TEST_F(PianoRollPresenterTest, FPR03_DeleteRemovesTheSelection)
{
    presenter.deleteSelection();
    EXPECT_FALSE(undoManager.canUndo());

    addNote(38, 960);
    addNote(42, 960);
    click(at(1000, 38));
    presenter.deleteSelection();

    ASSERT_EQ(pattern().numNotes(), 1);
    EXPECT_EQ(pattern().note(0).slotNote(), 42);
    EXPECT_EQ(presenter.numSelected(), 0);
}

// ----- Selection ----------------------------------------------------------------------------------

TEST_F(PianoRollPresenterTest, FPR05_CtrlAddsAndRemovesNotesFromTheSelection)
{
    addNote(38, 960);
    addNote(42, 960);
    presenter.setTool(PianoRollTool::select);
    EXPECT_EQ(presenter.tool(), PianoRollTool::select);

    click(at(1000, 38));
    click(at(1000, 42), kCtrl);
    EXPECT_EQ(presenter.numSelected(), 2);

    click(at(1000, 38), kCtrl);
    EXPECT_EQ(presenter.numSelected(), 1);
    EXPECT_FALSE(presenter.notes()[0].selected);

    click(at(1000, 38));
    EXPECT_EQ(presenter.numSelected(), 1);
    EXPECT_TRUE(presenter.notes()[0].selected);
}

TEST_F(PianoRollPresenterTest, FPR05_DraggingASelectedNoteMovesTheWholeSelection)
{
    addNote(38, 960);
    addNote(42, 1440);
    presenter.selectAll();

    drag(at(1000, 38), at(1240, 38));

    EXPECT_EQ(starts(), (std::vector<std::int64_t>{1200, 1680}));
}

TEST_F(PianoRollPresenterTest, FPR05_RectangleSelectsTheNotesItTouches)
{
    addNote(38, 960);
    addNote(42, 1440);
    addNote(36, 5000);
    presenter.setTool(PianoRollTool::select);

    presenter.mouseDown(at(2000, 36), PointerButton::left, kNone, 0);
    presenter.mouseDrag(at(1100, 42), kNone);
    const auto rect = presenter.selectionRect();
    ASSERT_TRUE(rect.has_value());
    EXPECT_EQ(rect->startTick, 1100);
    EXPECT_EQ(rect->endTick, 2000);
    EXPECT_EQ(rect->firstRow, row(42));
    EXPECT_EQ(rect->lastRow, row(36));
    presenter.mouseUp(at(1100, 42), kNone);

    EXPECT_FALSE(presenter.selectionRect().has_value());
    EXPECT_EQ(presenter.numSelected(), 2);

    drag(at(4900, 36), at(5100, 36), kCtrl);
    EXPECT_EQ(presenter.numSelected(), 3);
    drag(at(4900, 36), at(5100, 36), kCtrl); // already selected notes stay selected once
    EXPECT_EQ(presenter.numSelected(), 3);

    drag(at(4900, 36), at(5100, 36));
    EXPECT_EQ(presenter.numSelected(), 1);

    click(at(7000, 57));
    EXPECT_EQ(presenter.numSelected(), 0);
}

TEST_F(PianoRollPresenterTest, FPR05_RectangleOnlyTouchingRowsOrTimesSelectsNothing)
{
    addNote(38, 960);
    presenter.setTool(PianoRollTool::select);

    drag(at(0, 42), at(2000, 41));
    drag(at(0, 38), at(900, 38));
    drag(at(1200, 38), at(2000, 38));

    EXPECT_EQ(presenter.numSelected(), 0);
}

TEST_F(PianoRollPresenterTest, FPR05_StatusShowsSnapAndSelectedNotes)
{
    EXPECT_EQ(presenter.statusText(), "Snap: 1/16");
    addNote(38, 960);
    presenter.selectAll();
    EXPECT_EQ(presenter.statusText(), "Snap: 1/16 \xc2\xb7 1 Note ausgew\xc3\xa4hlt");
    addNote(42, 960);
    presenter.selectAll();
    presenter.setSnapEnabled(false);
    EXPECT_EQ(presenter.statusText(), "Snap: aus \xc2\xb7 2 Noten ausgew\xc3\xa4hlt");
}

TEST_F(PianoRollPresenterTest, FPR05_UndoRemovesDeletedNotesFromTheSelection)
{
    click(at(1000, 38));
    undoManager.undo();
    EXPECT_EQ(presenter.numSelected(), 0);
}

// ----- Clipboard ----------------------------------------------------------------------------------

class PianoRollClipboardTest : public PianoRollPresenterTest
{
protected:
    PianoRollClipboardTest()
    {
        addNote(38, 960);
        addNote(42, 1440);
        presenter.setTool(PianoRollTool::select);
    }
};

TEST_F(PianoRollClipboardTest, FPR05_PastesAtTheClickedPositionOneAfterAnother)
{
    EXPECT_FALSE(presenter.canPaste());
    presenter.paste();
    presenter.copy();
    EXPECT_FALSE(presenter.canPaste());
    EXPECT_FALSE(undoManager.canUndo());

    presenter.selectAll();
    presenter.copy();
    EXPECT_TRUE(presenter.canPaste());
    click(at(3900, 57));
    presenter.paste();
    EXPECT_EQ(presenter.numSelected(), 2);
    presenter.paste();

    EXPECT_EQ(starts(), (std::vector<std::int64_t>{960, 1440, 3840, 4320, 4560, 5040}));
    undoManager.undo();
    EXPECT_EQ(pattern().numNotes(), 4);
}

TEST_F(PianoRollClipboardTest, FPR05_NotesPastedBehindThePatternEndAreLeftOut)
{
    presenter.selectAll();
    presenter.copy();
    click(at(7300, 57));

    presenter.paste();

    EXPECT_EQ(starts(), (std::vector<std::int64_t>{960, 1440, 7200}));
}

TEST_F(PianoRollClipboardTest, FPR05_CutRemovesTheCopiedNotes)
{
    presenter.cut();
    EXPECT_FALSE(undoManager.canUndo());

    click(at(1000, 38));
    presenter.cut();
    EXPECT_EQ(pattern().numNotes(), 1);
    presenter.paste();
    EXPECT_EQ(starts(), (std::vector<std::int64_t>{960, 1440}));
}

TEST_F(PianoRollClipboardTest, FPR05_DuplicatePlacesTheCopyRightBehindTheSelection)
{
    presenter.duplicate();
    EXPECT_FALSE(undoManager.canUndo());

    presenter.selectAll();
    presenter.duplicate();

    EXPECT_EQ(starts(), (std::vector<std::int64_t>{960, 1440, 1680, 2160}));
    EXPECT_EQ(presenter.numSelected(), 2);
    EXPECT_TRUE(presenter.notes()[2].selected);
    undoManager.undo();
    EXPECT_EQ(pattern().numNotes(), 2);
}

TEST_F(PianoRollClipboardTest, FPR05_PasteNeedsAPattern)
{
    presenter.selectAll();
    presenter.copy();
    tree.getChildWithName(model::ids::patterns).removeAllChildren(nullptr);

    presenter.paste();
    presenter.duplicate();

    EXPECT_FALSE(undoManager.canUndo());
}

// ----- Velocity lane ------------------------------------------------------------------------------

TEST_F(PianoRollPresenterTest, FPR10_LaneShowsTheChosenRow)
{
    EXPECT_FALSE(presenter.laneRow().has_value());
    EXPECT_EQ(presenter.laneLabel(), "Lane: Zeile w\xc3\xa4hlen");

    presenter.setLaneRow(row(42));
    presenter.setLaneRow(-1);
    presenter.setLaneRow(19);
    EXPECT_EQ(presenter.laneRow(), row(42));
    EXPECT_EQ(presenter.laneLabel(), "Lane: Closed Hi-Hat (42)");

    addNote(38, 960);
    click(at(1000, 38));
    EXPECT_EQ(presenter.laneRow(), row(38));
}

TEST_F(PianoRollPresenterTest, FPR10_OneStrokeSetsTheVelocityOfSeveralNotes)
{
    addNote(42, 0);
    addNote(42, 480);
    addNote(42, 960);
    addNote(42, 2000);
    addNote(38, 480);
    presenter.setLaneRow(row(42));

    presenter.velocityDragBegin(0, 20, 10);
    presenter.velocityDrag(960, 120);

    EXPECT_EQ(pattern().note(0).velocity(), 20);
    EXPECT_EQ(pattern().note(1).velocity(), 70);
    EXPECT_EQ(pattern().note(2).velocity(), 120);
    EXPECT_EQ(pattern().note(3).velocity(), 100);
    EXPECT_EQ(pattern().note(4).velocity(), 100);

    undoManager.undo();
    EXPECT_EQ(pattern().note(0).velocity(), 100);
    EXPECT_EQ(pattern().note(2).velocity(), 100);
}

TEST_F(PianoRollPresenterTest, FPR10_StrokeBackwardsAndWithoutLaneRow)
{
    addNote(42, 0);
    addNote(42, 480);

    presenter.velocityDragBegin(480, 10, 0);
    EXPECT_EQ(pattern().note(1).velocity(), 100);

    presenter.setLaneRow(row(42));
    presenter.velocityDragBegin(480, 10, 0);
    presenter.velocityDrag(0, 50);
    EXPECT_EQ(pattern().note(0).velocity(), 50);
    EXPECT_EQ(pattern().note(1).velocity(), 10);
}

TEST_F(PianoRollPresenterTest, FPR10_VelocityFromTheHeightInTheLane)
{
    EXPECT_EQ(PianoRollPresenter::velocityAt(0.0, 100.0), 127);
    EXPECT_EQ(PianoRollPresenter::velocityAt(50.0, 100.0), 64);
    EXPECT_EQ(PianoRollPresenter::velocityAt(100.0, 100.0), 1);
    EXPECT_EQ(PianoRollPresenter::velocityAt(-20.0, 100.0), 127);
    EXPECT_DOUBLE_EQ(PianoRollPresenter::yOfVelocity(127, 100.0), 0.0);
    EXPECT_DOUBLE_EQ(PianoRollPresenter::yOfVelocity(0, 100.0), 100.0);
}

// ----- Inspector ----------------------------------------------------------------------------------

TEST_F(PianoRollPresenterTest, FPR05_InspectorShowsTheSelectedNote)
{
    EXPECT_FALSE(presenter.noteDetails().has_value());

    addNote(38, 4800, 240, 90);
    presenter.selectAll();
    const auto details = presenter.noteDetails();

    ASSERT_TRUE(details.has_value());
    EXPECT_EQ(details->instrument, "38 \xc2\xb7 Acoustic Snare");
    EXPECT_EQ(details->position, "002.2.000 \xc2\xb7 4800 Ticks");
    EXPECT_EQ(details->length, "1/16 \xc2\xb7 240 Ticks");
    EXPECT_EQ(details->velocity, 90);

    pattern().note(0).setLengthTicks(250);
    EXPECT_EQ(presenter.noteDetails()->length, "250 Ticks");
}

TEST_F(PianoRollPresenterTest, FPR05_InspectorSumsUpSeveralNotes)
{
    addNote(38, 960, 240, 90);
    addNote(42, 960, 240, 80);
    presenter.selectAll();

    const auto details = presenter.noteDetails();

    ASSERT_TRUE(details.has_value());
    EXPECT_EQ(details->instrument, "2 Noten");
    EXPECT_EQ(details->position, "\xe2\x80\x93");
    EXPECT_EQ(details->velocity, 90);
}

TEST_F(PianoRollPresenterTest, FPR10_InspectorVelocityChangesAllSelectedNotesInOneStep)
{
    presenter.setSelectedVelocity(10);
    EXPECT_FALSE(undoManager.canUndo());

    addNote(38, 960, 240, 90);
    addNote(42, 960, 240, 80);
    presenter.selectAll();
    presenter.setSelectedVelocity(50);
    presenter.setSelectedVelocity(60);
    EXPECT_EQ(pattern().note(0).velocity(), 60);
    EXPECT_EQ(pattern().note(1).velocity(), 60);
    presenter.endVelocityEdit();
    presenter.setSelectedVelocity(70);

    undoManager.undo();
    EXPECT_EQ(pattern().note(0).velocity(), 60);
    undoManager.undo();
    EXPECT_EQ(pattern().note(0).velocity(), 90);
    EXPECT_EQ(pattern().note(1).velocity(), 80);
}

TEST_F(PianoRollPresenterTest, FPR10_AnotherEditEndsTheInspectorVelocityStep)
{
    addNote(38, 960, 240, 90);
    presenter.selectAll();
    presenter.setSelectedVelocity(50);
    undoManager.beginNewTransaction("Umbenennen");
    model::Project{tree, &undoManager}.setName("Rock-Demo");
    presenter.setSelectedVelocity(60);

    undoManager.undo();
    EXPECT_EQ(pattern().note(0).velocity(), 50);
}

// ----- Live display and pattern switch ------------------------------------------------------------

TEST_F(PianoRollPresenterTest, FIN10_RecordedNotesShowAtOnce)
{
    const auto before = presenter.changeCount();
    addNote(38, 1000, 240, 100, NoteOrigin::live);
    addNote(38, kLength); // behind the end

    EXPECT_NE(presenter.changeCount(), before);
    ASSERT_EQ(presenter.notes().size(), 1U);
    EXPECT_EQ(presenter.notes()[0].origin, NoteOrigin::live);
    EXPECT_EQ(presenter.notes()[0].row, row(38));
}

TEST_F(PianoRollPresenterTest, FSO01_OpeningAnotherPatternShowsItsNotes)
{
    addNote(38, 960);
    presenter.selectAll();
    project.addPattern("p-2", "Fill", 1).addNote({42, 0, 240, 100, NoteOrigin::grid});

    active.select(1);

    EXPECT_EQ(presenter.numSelected(), 0);
    ASSERT_EQ(presenter.notes().size(), 1U);
    EXPECT_EQ(presenter.notes()[0].row, row(42));
    EXPECT_EQ(presenter.lengthTicks(), 3840);
}

TEST_F(PianoRollPresenterTest, FPR03_DraggingWithoutButtonDoesNothing)
{
    presenter.mouseDrag(at(1000, 38), kNone);
    presenter.mouseUp(at(1000, 38), kNone);
    EXPECT_EQ(pattern().numNotes(), 0);
}

} // namespace
} // namespace drumprog::ui
