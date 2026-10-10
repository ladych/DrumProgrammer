#include "ui/SongTimelinePresenter.h"

#include "model/FakeIdGenerator.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

class SongTimelinePresenterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        project.addPattern("p-2", "Fill", 1).setColour("#3B82F6");
        undoManager.clearUndoHistory();
    }

    /// Start bars of the blocks in timeline order.
    [[nodiscard]] std::vector<int> starts() const
    {
        std::vector<int> bars;
        for (const auto& block : presenter.blocks())
            bars.push_back(block.startBar);
        return bars;
    }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault(); // "Pattern 1" (id-1), 2 bars
    model::Project project{tree, nullptr};
    juce::UndoManager undoManager;
    ActivePattern active{tree};
    SongTimelinePresenter presenter{tree, undoManager, active};
};

TEST_F(SongTimelinePresenterTest, FSO02_ShowsTheBlocksWithNameColourAndLength)
{
    project.song().addEntry("p-2", 2);
    project.song().addEntry("id-1", 0);

    const auto blocks = presenter.blocks();

    ASSERT_EQ(blocks.size(), 2U);
    EXPECT_EQ(blocks[0].patternIndex, 0);
    EXPECT_EQ(blocks[0].startBar, 0);
    EXPECT_EQ(blocks[0].lengthBars, 2);
    EXPECT_EQ(blocks[0].name, "Pattern 1");
    EXPECT_EQ(blocks[0].colour, "#E8743B");
    EXPECT_FALSE(blocks[0].selected);
    EXPECT_EQ(blocks[1].patternIndex, 1);
    EXPECT_EQ(blocks[1].startBar, 2);
    EXPECT_EQ(blocks[1].lengthBars, 1);
    EXPECT_EQ(blocks[1].name, "Fill");
    EXPECT_EQ(blocks[1].colour, "#3B82F6");
    EXPECT_EQ(presenter.songLengthBars(), 3);
    EXPECT_EQ(presenter.ticksPerBar(), 3840);
    EXPECT_FALSE(presenter.isEmpty());
}

TEST_F(SongTimelinePresenterTest, FSO02_EmptySong)
{
    EXPECT_TRUE(presenter.blocks().empty());
    EXPECT_TRUE(presenter.isEmpty());
    EXPECT_EQ(presenter.songLengthBars(), 0);
    EXPECT_EQ(presenter.blockAt(0), -1);
}

TEST_F(SongTimelinePresenterTest, FSO02_DroppingAPatternPlacesABlockAndSelectsIt)
{
    presenter.insertPattern(1, 4);
    presenter.insertPattern(0, -3);
    presenter.insertPattern(2, 0); // no such pattern

    EXPECT_EQ(starts(), (std::vector<int>{0, 4}));
    EXPECT_EQ(presenter.selectedBlock(), 0);
    EXPECT_TRUE(presenter.blocks()[0].selected);

    undoManager.undo();
    EXPECT_EQ(starts(), (std::vector<int>{4}));
    EXPECT_EQ(undoManager.getUndoDescription(), juce::String::fromUTF8("Block einf\xc3\xbcgen"));
}

TEST_F(SongTimelinePresenterTest, FSO02_InsertAppendsTheActivePatternBehindTheSong)
{
    presenter.appendActivePattern();
    active.select(1);
    presenter.appendActivePattern();

    ASSERT_EQ(starts(), (std::vector<int>{0, 2}));
    EXPECT_EQ(presenter.blocks()[1].patternIndex, 1);
    EXPECT_EQ(presenter.selectedBlock(), 1);
}

TEST_F(SongTimelinePresenterTest, FSO03_BlockAtFindsTheTopmostBlock)
{
    project.song().addEntry("id-1", 0);
    project.song().addEntry("p-2", 1);

    EXPECT_EQ(presenter.blockAt(0), 0);
    EXPECT_EQ(presenter.blockAt(1), 1);
    EXPECT_EQ(presenter.blockAt(2), -1);
    EXPECT_EQ(presenter.blockAt(-1), -1);
}

TEST_F(SongTimelinePresenterTest, FSO03_SelectionFollowsTheBlock)
{
    project.song().addEntry("id-1", 4);
    presenter.select(0);
    project.song().addEntry("p-2", 0);

    EXPECT_EQ(presenter.selectedBlock(), 1);
    presenter.select(5);
    EXPECT_EQ(presenter.selectedBlock(), -1);
    presenter.select(-1);
    EXPECT_EQ(presenter.selectedBlock(), -1);
}

TEST_F(SongTimelinePresenterTest, FSO03_DeletesTheSelectedBlock)
{
    project.song().addEntry("id-1", 0);
    project.song().addEntry("p-2", 2);
    presenter.removeSelected();
    EXPECT_EQ(starts(), (std::vector<int>{0, 2}));

    presenter.select(1);
    presenter.removeSelected();

    EXPECT_EQ(starts(), (std::vector<int>{0}));
    EXPECT_EQ(presenter.selectedBlock(), -1);
    undoManager.undo();
    EXPECT_EQ(starts(), (std::vector<int>{0, 2}));
}

TEST_F(SongTimelinePresenterTest, FSO03_ClearEmptiesTheSongInOneStep)
{
    project.song().addEntry("id-1", 0);
    project.song().addEntry("p-2", 2);

    presenter.clear();

    EXPECT_TRUE(presenter.isEmpty());
    undoManager.undo();
    EXPECT_EQ(starts(), (std::vector<int>{0, 2}));
}

TEST_F(SongTimelinePresenterTest, FSO06_OpeningABlockShowsItsPatternInThePianoRoll)
{
    project.song().addEntry("p-2", 0);
    presenter.open(0);
    EXPECT_EQ(active.index(), 1);
    presenter.open(3);
    presenter.open(-1);
    EXPECT_EQ(active.index(), 1);
}

TEST_F(SongTimelinePresenterTest, FSO03_DragMovesTheBlockBarByBar)
{
    project.song().addEntry("id-1", 0);
    project.song().addEntry("p-2", 4);

    presenter.press(0, 1); // grabbed in its second bar
    EXPECT_EQ(presenter.selectedBlock(), 0);
    EXPECT_FALSE(presenter.ghost().has_value());
    presenter.dragTo(7, false);

    const auto ghost = presenter.ghost();
    ASSERT_TRUE(ghost.has_value());
    EXPECT_EQ(ghost->patternIndex, 0);
    EXPECT_EQ(ghost->startBar, 6);
    EXPECT_EQ(ghost->lengthBars, 2);
    EXPECT_FALSE(ghost->duplicate);
    EXPECT_EQ(starts(), (std::vector<int>{0, 4})); // only a preview until release

    presenter.release();
    EXPECT_FALSE(presenter.ghost().has_value());
    EXPECT_EQ(starts(), (std::vector<int>{4, 6}));
    EXPECT_EQ(presenter.selectedBlock(), 1);

    undoManager.undo();
    EXPECT_EQ(starts(), (std::vector<int>{0, 4}));
    EXPECT_EQ(undoManager.getRedoDescription(), "Block verschieben");
}

TEST_F(SongTimelinePresenterTest, FSO03_BlocksCannotMoveBeforeTheSongStart)
{
    project.song().addEntry("id-1", 2);
    presenter.press(0, 3);
    presenter.dragTo(0, false);
    EXPECT_EQ(presenter.ghost()->startBar, 0);
    presenter.release();
    EXPECT_EQ(starts(), (std::vector<int>{0}));
}

TEST_F(SongTimelinePresenterTest, FSO03_AltDragDuplicatesTheBlock)
{
    project.song().addEntry("p-2", 0);

    presenter.press(0, 0);
    presenter.dragTo(3, true);
    EXPECT_TRUE(presenter.ghost()->duplicate);
    presenter.release();

    EXPECT_EQ(starts(), (std::vector<int>{0, 3}));
    EXPECT_EQ(presenter.blocks()[1].patternIndex, 1);
    EXPECT_EQ(presenter.selectedBlock(), 1);
    EXPECT_EQ(undoManager.getUndoDescription(), "Block duplizieren");
}

TEST_F(SongTimelinePresenterTest, FSO03_ReleasingWhereTheBlockWasChangesNothing)
{
    project.song().addEntry("p-2", 2);

    presenter.press(0, 2);
    presenter.dragTo(5, true);
    presenter.dragTo(2, true);
    EXPECT_FALSE(presenter.ghost().has_value());
    presenter.release();

    EXPECT_EQ(starts(), (std::vector<int>{2}));
    EXPECT_FALSE(undoManager.canUndo());
}

TEST_F(SongTimelinePresenterTest, FSO03_PressingBesideTheBlocksStartsNoDrag)
{
    project.song().addEntry("p-2", 2);
    presenter.select(0);

    presenter.press(-1, 0);
    presenter.dragTo(4, false);
    presenter.release();
    presenter.dragTo(4, false);

    EXPECT_EQ(presenter.selectedBlock(), -1);
    EXPECT_FALSE(presenter.ghost().has_value());
    EXPECT_EQ(starts(), (std::vector<int>{2}));
}

TEST_F(SongTimelinePresenterTest, FSO03_ADragOfABlockRemovedMeanwhileIsDropped)
{
    project.song().addEntry("p-2", 2);
    presenter.press(0, 2);
    presenter.dragTo(5, false);
    project.song().clear();

    presenter.release();

    EXPECT_TRUE(presenter.isEmpty());
}

TEST_F(SongTimelinePresenterTest, FSO02_HoveringAPatternShowsWhereItLands)
{
    presenter.hoverPattern(1, 3);
    auto ghost = presenter.ghost();
    ASSERT_TRUE(ghost.has_value());
    EXPECT_EQ(ghost->patternIndex, 1);
    EXPECT_EQ(ghost->startBar, 3);
    EXPECT_EQ(ghost->lengthBars, 1);

    presenter.hoverPattern(0, -2);
    EXPECT_EQ(presenter.ghost()->startBar, 0);
    presenter.hoverPattern(9, 2);
    EXPECT_FALSE(presenter.ghost().has_value());
    presenter.hoverPattern(-1, 2);
    EXPECT_FALSE(presenter.ghost().has_value());

    presenter.hoverPattern(1, 3);
    presenter.endHover();
    EXPECT_FALSE(presenter.ghost().has_value());
}

TEST_F(SongTimelinePresenterTest, FSO04_ChangingAPatternChangesAllItsBlocks)
{
    project.song().addEntry("p-2", 0);
    project.song().addEntry("p-2", 4);
    const auto before = presenter.changeCount();

    project.pattern(1).setLengthBars(3);
    project.pattern(1).setName("Big Fill");

    EXPECT_NE(presenter.changeCount(), before);
    for (const auto& block : presenter.blocks())
    {
        EXPECT_EQ(block.lengthBars, 3);
        EXPECT_EQ(block.name, "Big Fill");
    }
}

} // namespace
} // namespace drumprog::ui
