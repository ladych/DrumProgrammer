#include "ui/BackingTrackPresenter.h"

#include "io/Utf8Path.h"
#include "model/FakeIdGenerator.h"
#include "model/ProjectFactory.h"
#include "ui/FakeBackingTrackLoader.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace drumprog::ui
{
namespace
{

constexpr double kRate = 48000.0;
// 120 BPM in 4/4: one bar = 2 s = 96000 frames at 48 kHz.
constexpr double kBarFrames = 96000.0;

class BackingTrackPresenterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        project().setBpm(120.0);
        loader.add(song, kRate, static_cast<int>(2.5 * kBarFrames));
        loader.add(other, kRate / 2, 1000);
        player.prepare(kRate);
    }

    [[nodiscard]] model::Project project() { return {tree, &undoManager}; }

    /// The audio thread takes the latest stream; returns its end in device samples.
    std::int64_t playerEnd()
    {
        player.update();
        player.tick();
        return player.endSample(0);
    }

    struct Player : engine::BackingTrackPlayer
    {
        void tick() { collectGarbage(); }
    };

    const std::filesystem::path song = std::filesystem::path{"music"} / "song.wav";
    const std::filesystem::path other = std::filesystem::path{"music"} / "other.wav";
    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    juce::UndoManager undoManager;
    FakeBackingTrackLoader loader;
    Player player;
    BackingTrackPresenter presenter{tree, undoManager, loader, player};
};

TEST_F(BackingTrackPresenterTest, FBT01_ANewProjectHasNoTrack)
{
    EXPECT_FALSE(presenter.hasTrack());
    EXPECT_FALSE(presenter.isMissing());
    EXPECT_EQ(presenter.label(), "");
    EXPECT_EQ(presenter.lengthSeconds(), 0.0);
    EXPECT_EQ(presenter.startBar(), 0.0);
    EXPECT_EQ(presenter.endBar(), 0.0);
    EXPECT_EQ(presenter.lengthBars(), 0);
    EXPECT_EQ(presenter.offsetMs(), 0.0);
    EXPECT_EQ(playerEnd(), 0);
}

TEST_F(BackingTrackPresenterTest, FBT01_LoadingSetsTheFileAndHandsTheStreamToThePlayer)
{
    const auto changes = presenter.changeCount();
    ASSERT_TRUE(presenter.load(song));
    EXPECT_GT(presenter.changeCount(), changes);
    EXPECT_TRUE(presenter.hasTrack());
    EXPECT_EQ(presenter.file(), song);
    EXPECT_EQ(project().backingTrack().filePath(), io::utf8FromPath(song));
    EXPECT_EQ(presenter.label(), "song.wav");
    EXPECT_DOUBLE_EQ(presenter.lengthSeconds(), 5.0);
    EXPECT_EQ(playerEnd(), static_cast<std::int64_t>(2.5 * kBarFrames));
    EXPECT_EQ(loader.opened, 1); // not opened a second time by the model change
    EXPECT_EQ(undoManager.getUndoDescription(), "Backing-Track laden");
}

TEST_F(BackingTrackPresenterTest, FBT01_AFileThatCannotBeReadKeepsTheTrack)
{
    presenter.load(song);
    EXPECT_FALSE(presenter.load("broken.wav"));
    EXPECT_EQ(presenter.file(), song);
}

TEST_F(BackingTrackPresenterTest, FBT01_RemoveIsOneUndoStepAndUndoBringsTheTrackBack)
{
    presenter.load(song);
    model::Project{tree, nullptr}.backingTrack().setOffsetSamples(100);
    presenter.remove();
    EXPECT_FALSE(presenter.hasTrack());
    EXPECT_EQ(project().backingTrack().offsetSamples(), 0);
    EXPECT_EQ(playerEnd(), 0);
    undoManager.undo();
    EXPECT_EQ(presenter.file(), song);
    EXPECT_GT(playerEnd(), 0);
    undoManager.undo();
    EXPECT_FALSE(presenter.hasTrack());

    const auto steps = undoManager.getNumActionsInCurrentTransaction();
    presenter.remove(); // nothing to remove
    EXPECT_EQ(undoManager.getNumActionsInCurrentTransaction(), steps);
}

TEST_F(BackingTrackPresenterTest, FPJ03_AMissingFileIsShownAsMissing)
{
    project().backingTrack().setFilePath("weg.wav");
    EXPECT_TRUE(presenter.hasTrack());
    EXPECT_TRUE(presenter.isMissing());
    EXPECT_EQ(presenter.label(), "weg.wav (Datei fehlt)");
    EXPECT_EQ(presenter.lengthBars(), 0);
    EXPECT_EQ(playerEnd(), 0);
    presenter.setOffsetMs(100.0);
    presenter.beginOffsetDrag();
    presenter.dragOffsetBy(1.0);
    presenter.endOffsetDrag();
    EXPECT_EQ(project().backingTrack().offsetSamples(), 0);
}

TEST_F(BackingTrackPresenterTest, FPJ02_AnOpenedProjectOpensItsTrack)
{
    model::Project{tree, nullptr}.backingTrack().setFilePath(io::utf8FromPath(other));
    EXPECT_FALSE(presenter.isMissing());
    EXPECT_EQ(playerEnd(), 2000); // 24 kHz file on 48 kHz
}

TEST_F(BackingTrackPresenterTest, FBT04_FBT05_PositionOnTheTimelineFollowsTheOffset)
{
    presenter.load(song);
    EXPECT_DOUBLE_EQ(presenter.startBar(), 0.0);
    EXPECT_DOUBLE_EQ(presenter.endBar(), 2.5);
    EXPECT_EQ(presenter.lengthBars(), 3);

    presenter.setOffsetMs(1000.0); // the lead-in of 1 s is skipped
    EXPECT_EQ(project().backingTrack().offsetSamples(), 48000);
    EXPECT_DOUBLE_EQ(presenter.offsetMs(), 1000.0);
    EXPECT_DOUBLE_EQ(presenter.startBar(), -0.5);
    EXPECT_DOUBLE_EQ(presenter.endBar(), 2.0);
    EXPECT_EQ(presenter.lengthBars(), 2);
    EXPECT_DOUBLE_EQ(presenter.fileSecondsAtBar(0.0), 1.0);
    EXPECT_DOUBLE_EQ(presenter.fileSecondsAtBar(1.5), 4.0);

    presenter.setOffsetMs(-6000.0);
    EXPECT_DOUBLE_EQ(presenter.endBar(), 5.5);
    presenter.setOffsetMs(10000.0);
    EXPECT_EQ(presenter.lengthBars(), 0);
}

TEST_F(BackingTrackPresenterTest, FBT05_OffsetChangesInARowAreOneUndoStep)
{
    presenter.load(song);
    presenter.setOffsetMs(10.0);
    presenter.setOffsetMs(20.0);
    presenter.setOffsetMs(30.0);
    undoManager.undo();
    EXPECT_EQ(project().backingTrack().offsetSamples(), 0);
    EXPECT_EQ(presenter.file(), song);
}

TEST_F(BackingTrackPresenterTest, FBT05_DraggingTheWaveformMovesTheTrackInOneUndoStep)
{
    presenter.load(song);
    presenter.setOffsetMs(1000.0);
    presenter.dragOffsetBy(1.0); // no drag running
    presenter.endOffsetDrag();
    EXPECT_EQ(project().backingTrack().offsetSamples(), 48000);

    presenter.beginOffsetDrag();
    presenter.dragOffsetBy(0.25);
    presenter.dragOffsetBy(0.5); // half a bar later = 1 s
    presenter.endOffsetDrag();
    EXPECT_EQ(project().backingTrack().offsetSamples(), 0);
    EXPECT_DOUBLE_EQ(presenter.startBar(), 0.0);

    undoManager.undo();
    EXPECT_EQ(project().backingTrack().offsetSamples(), 48000);
}

TEST_F(BackingTrackPresenterTest, FBT04_TheTempoChangesTheBars)
{
    presenter.load(song);
    project().setBpm(60.0);
    EXPECT_DOUBLE_EQ(presenter.endBar(), 1.25);
}

TEST_F(BackingTrackPresenterTest, Q04_TickFreesReplacedStreams)
{
    presenter.load(song);
    player.update();
    presenter.load(other);
    player.update(); // the first stream waits in the retire slot
    presenter.tick();
    player.update();
    EXPECT_EQ(player.endSample(0), 2000);
}

} // namespace
} // namespace drumprog::ui
