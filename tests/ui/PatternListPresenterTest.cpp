#include "ui/PatternListPresenter.h"

#include "model/FakeIdGenerator.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

class PatternListPresenterTest : public ::testing::Test
{
protected:
    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault(); // "Pattern 1", id-1
    model::Project project{tree, nullptr};
    juce::UndoManager undoManager;
    ActivePattern active{tree};
    PatternListPresenter presenter{tree, undoManager, ids, active};
};

TEST_F(PatternListPresenterTest, FSO01_ShowsNameColourAndLength)
{
    project.addPattern("p-2", "Fill", 1);

    ASSERT_EQ(presenter.numPatterns(), 2);
    EXPECT_EQ(presenter.name(0), "Pattern 1");
    EXPECT_EQ(presenter.colour(0), "#E8743B");
    EXPECT_EQ(presenter.lengthBars(0), 2);
    EXPECT_EQ(presenter.lengthText(0), "2 Takte");
    EXPECT_EQ(presenter.lengthText(1), "1 Takt");
}

TEST_F(PatternListPresenterTest, FSO01_RowsOutsideTheListAreEmpty)
{
    EXPECT_EQ(presenter.name(1), "");
    EXPECT_EQ(presenter.colour(-1), "");
    EXPECT_EQ(presenter.lengthBars(1), 0);
    EXPECT_EQ(presenter.lengthText(1), "");
}

TEST_F(PatternListPresenterTest, FSO01_AddAppendsANumberedPatternAndOpensIt)
{
    presenter.add();

    ASSERT_EQ(presenter.numPatterns(), 2);
    EXPECT_EQ(presenter.name(1), "Pattern 2");
    EXPECT_EQ(presenter.lengthBars(1), 2);
    EXPECT_EQ(presenter.colour(1), PatternListPresenter::palette()[1]);
    EXPECT_EQ(presenter.selectedIndex(), 1);

    undoManager.undo();
    EXPECT_EQ(presenter.numPatterns(), 1);
    EXPECT_EQ(presenter.selectedIndex(), 0);
}

TEST_F(PatternListPresenterTest, FSO01_NewNamesAreUnique)
{
    project.pattern(0).setName("Pattern 2");
    presenter.add();
    EXPECT_EQ(presenter.name(1), "Pattern 3");
}

TEST_F(PatternListPresenterTest, FSO01_ColoursOfNewPatternsRepeat)
{
    for (std::size_t count = 1; count <= PatternListPresenter::palette().size(); ++count)
        presenter.add();
    EXPECT_EQ(presenter.colour(8), PatternListPresenter::palette()[0]);
}

TEST_F(PatternListPresenterTest, FSO01_RenamesWithTrimmedNames)
{
    presenter.rename(0, "  Verse Groove ");
    EXPECT_EQ(presenter.name(0), "Verse Groove");

    presenter.rename(0, "   ");
    presenter.rename(5, "Chorus");
    EXPECT_EQ(presenter.name(0), "Verse Groove");

    undoManager.undo();
    EXPECT_EQ(presenter.name(0), "Pattern 1");
}

TEST_F(PatternListPresenterTest, FSO01_ChoosesAColour)
{
    presenter.setColour(0, "#22C55E");
    presenter.setColour(3, "#3B82F6");
    EXPECT_EQ(presenter.colour(0), "#22C55E");
}

TEST_F(PatternListPresenterTest, FPR09_LengthIsOneToSixtyFourBars)
{
    presenter.setLengthBars(0, 8);
    EXPECT_EQ(presenter.lengthBars(0), 8);
    presenter.setLengthBars(0, 65);
    EXPECT_EQ(presenter.lengthBars(0), 64);
    presenter.setLengthBars(0, 0);
    EXPECT_EQ(presenter.lengthBars(0), 1);
    presenter.setLengthBars(2, 4);

    undoManager.undo();
    EXPECT_EQ(presenter.lengthBars(0), 64);
}

TEST_F(PatternListPresenterTest, FSO01_DuplicatesBehindTheOriginalAndOpensTheCopy)
{
    project.addPattern("p-2", "Fill", 1);
    project.pattern(0).addNote({38, 960, 240, 90, model::NoteOrigin::grid});

    presenter.duplicate(0);
    presenter.duplicate(7);

    ASSERT_EQ(presenter.numPatterns(), 3);
    EXPECT_EQ(presenter.name(1), "Pattern 1 Kopie");
    EXPECT_EQ(project.pattern(1).numNotes(), 1);
    EXPECT_NE(project.pattern(1).id(), project.pattern(0).id());
    EXPECT_EQ(presenter.selectedIndex(), 1);
}

TEST_F(PatternListPresenterTest, FSO01_RemovesPatternsButNeverTheLast)
{
    presenter.add();
    EXPECT_TRUE(presenter.canRemove());

    presenter.remove(0);
    ASSERT_EQ(presenter.numPatterns(), 1);
    EXPECT_EQ(presenter.name(0), "Pattern 2");
    EXPECT_FALSE(presenter.canRemove());

    presenter.remove(0);
    presenter.remove(3);
    EXPECT_EQ(presenter.numPatterns(), 1);

    undoManager.undo();
    EXPECT_EQ(presenter.numPatterns(), 2);
}

TEST_F(PatternListPresenterTest, FSO01_ClickOpensThePattern)
{
    presenter.add();
    presenter.select(0);
    EXPECT_EQ(presenter.selectedIndex(), 0);
    EXPECT_EQ(active.index(), 0);
}

TEST_F(PatternListPresenterTest, FSO01_ChangeCountFollowsTheModel)
{
    const auto before = presenter.changeCount();
    project.pattern(0).setName("Intro");
    EXPECT_NE(presenter.changeCount(), before);
}

TEST_F(PatternListPresenterTest, FSO05_DeletingAPatternOfTheSongNeedsConfirmation)
{
    project.addPattern("p-2", "Verse", 2);
    project.song().addEntry("p-2", 0);
    project.song().addEntry("id-1", 2);
    project.song().addEntry("p-2", 4);

    EXPECT_EQ(presenter.songUses(1), 2);
    EXPECT_EQ(presenter.songUses(0), 1);
    EXPECT_EQ(presenter.songUses(4), 0);
    EXPECT_EQ(presenter.removeQuestion(1),
              "\xe2\x80\x9eVerse\xe2\x80\x9c wird im Song 2-mal verwendet. Beim L\xc3\xb6schen werden alle "
              "Bl\xc3\xb6"
              "cke entfernt.");
    EXPECT_EQ(presenter.removeQuestion(0),
              "\xe2\x80\x9ePattern 1\xe2\x80\x9c wird im Song 1-mal verwendet. Beim L\xc3\xb6schen werden "
              "alle Bl\xc3\xb6"
              "cke entfernt.");

    presenter.remove(1);
    EXPECT_EQ(project.song().numEntries(), 1);
    EXPECT_EQ(presenter.removeQuestion(0),
              "\xe2\x80\x9ePattern 1\xe2\x80\x9c wird im Song 1-mal verwendet. Beim L\xc3\xb6schen werden "
              "alle Bl\xc3\xb6"
              "cke entfernt.");
}

TEST_F(PatternListPresenterTest, FSO05_DeletingAnUnusedPatternNeedsNoConfirmation)
{
    project.addPattern("p-2", "Verse", 2);
    EXPECT_EQ(presenter.removeQuestion(1), "");
    EXPECT_EQ(presenter.removeQuestion(7), "");
}

} // namespace
} // namespace drumprog::ui
