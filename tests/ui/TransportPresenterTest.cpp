#include "ui/TransportPresenter.h"

#include "io/MockFileSystem.h"
#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "model/SnapshotBuilder.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <string>

namespace drumprog::ui
{
namespace
{

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

constexpr double kRate = 48000.0;
constexpr int kTick = 25;          // samples at 120 BPM
constexpr int kBar = 3840 * kTick; // 4/4

class TransportPresenterTest : public ::testing::Test
{
protected:
    void SetUp() override { sequencer.prepare(kRate); }

    /// Plays the role of the audio thread for one block.
    void audioBlock(int numSamples = 480)
    {
        const model::Project project{tree, nullptr};
        snapshot = model::SnapshotBuilder::build(project, project.activeKit(globalKit));
        sequencer.process(snapshot.get(), numSamples, block);
    }

    /// One block in which a hit is played at the block start.
    void audioBlockWithHit(int slotIndex)
    {
        audioBlock();
        const std::array hits{engine::LiveHit{slotIndex, 100, 50.0}};
        sequencer.record(hits, 50.0);
    }

    [[nodiscard]] model::Pattern pattern() const { return model::Project{tree, nullptr}.pattern(0); }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    juce::ValueTree globalKit = model::ProjectFactory::createDefaultKit();
    juce::UndoManager undoManager;
    model::TakeRecorder recorder{tree, globalKit, undoManager};
    NiceMock<io::MockFileSystem> fileSystem;
    io::SettingsStore offsetSettings{fileSystem, "recording-offset.txt"};
    engine::Sequencer sequencer;
    engine::Metronome metronome;
    TransportPresenter transport{tree, sequencer, metronome, recorder, offsetSettings};
    std::unique_ptr<const engine::ProjectSnapshot> snapshot;
    engine::SequencerBlock block;
};

TEST_F(TransportPresenterTest, FTR01_SpaceTogglesPlayAndStop)
{
    EXPECT_FALSE(transport.isPlaying());
    transport.togglePlay();
    audioBlock();
    EXPECT_TRUE(transport.isPlaying());
    transport.togglePlay();
    audioBlock();
    EXPECT_FALSE(transport.isPlaying());
}

TEST_F(TransportPresenterTest, FTR01_RewindGoesBackToTheStart)
{
    transport.play();
    audioBlock(kBar / 2);
    EXPECT_EQ(transport.positionText(), "001.3.000");
    transport.rewind();
    audioBlock(0);
    EXPECT_EQ(transport.positionText(), "001.1.000");
}

TEST_F(TransportPresenterTest, FTR02_LoopIsForwardedToTheSequencer)
{
    EXPECT_TRUE(transport.loop());
    transport.setLoop(false);
    EXPECT_FALSE(transport.loop());
    EXPECT_FALSE(sequencer.loop());
}

TEST_F(TransportPresenterTest, FTR04_PositionIsBarBeatTick)
{
    transport.play();
    audioBlock((3840 + 960 + 90) * kTick);
    EXPECT_EQ(transport.positionText(), "002.2.090");

    model::Project{tree, nullptr}.setTimeSignature({6, 8});
    EXPECT_EQ(transport.positionText(), "002.5.090");
}

TEST_F(TransportPresenterTest, FPR11_PlayheadFollowsThePositionWhilePlaying)
{
    EXPECT_FALSE(transport.playheadTick().has_value());
    transport.play();
    audioBlock(1000 * kTick);
    EXPECT_EQ(transport.playheadTick(), 1000);
    transport.stop();
    audioBlock();
    EXPECT_FALSE(transport.playheadTick().has_value());
}

TEST_F(TransportPresenterTest, FSO01_ChoosingAnotherPatternSwitchesTheRunningPlayback)
{
    model::Project{tree, nullptr}.addPattern("p-2", "Fill", 1);
    transport.play();
    audioBlock((3840 + 1920) * kTick);

    transport.setActivePattern(1);
    audioBlock(0);

    EXPECT_TRUE(transport.isPlaying());
    EXPECT_EQ(transport.positionText(), "001.3.000"); // wrapped into the one-bar pattern
    transport.setActivePattern(1);
    audioBlock(0);
    EXPECT_EQ(transport.positionText(), "001.3.000");
}

TEST_F(TransportPresenterTest, FSO01_ChoosingAnotherPatternKeepsARunningRecording)
{
    model::Project{tree, nullptr}.addPattern("p-2", "Fill", 1);
    transport.setCountInBars(0);
    transport.toggleRecordArmed();
    transport.play();
    audioBlock(1000 * kTick);

    transport.setActivePattern(1);
    audioBlock(0);

    EXPECT_EQ(transport.activePattern(), 1);
    EXPECT_TRUE(transport.isRecording());
    EXPECT_EQ(transport.playheadTick(), 1000);
    transport.stop();
}

// ----- Song mode (F-TR-05) -------------------------------------------------------------------

class TransportPresenterSongTest : public TransportPresenterTest
{
protected:
    void SetUp() override
    {
        TransportPresenterTest::SetUp();
        // "Fill" (1 bar) on bar 1, "Pattern 1" (2 bars) on bar 2.
        model::Project project{tree, nullptr};
        project.addPattern("p-2", "Fill", 1);
        project.song().addEntry("p-2", 0);
        project.song().addEntry("id-1", 1);
    }
};

TEST_F(TransportPresenterSongTest, FTR05_SongModePlaysTheTimeline)
{
    EXPECT_EQ(transport.playMode(), PlayMode::pattern);
    transport.setPlayMode(PlayMode::song);
    EXPECT_EQ(transport.playMode(), PlayMode::song);
    EXPECT_FALSE(transport.songPlayheadTick().has_value());

    transport.play();
    audioBlock((3840 + 960) * kTick);

    EXPECT_EQ(transport.positionText(), "002.2.000");
    EXPECT_EQ(transport.songPlayheadTick(), 3840 + 960);
    EXPECT_EQ(transport.playheadTick(), 960); // inside the block of the active "Pattern 1"
}

TEST_F(TransportPresenterSongTest, FPR11_PianoRollPlayheadOnlyWhileABlockOfTheActivePatternPlays)
{
    transport.setPlayMode(PlayMode::song);
    transport.play();
    audioBlock(0);
    EXPECT_EQ(transport.songPlayheadTick(), 0);
    EXPECT_FALSE(transport.playheadTick().has_value()); // "Fill" plays

    audioBlock((3840 + 960) * kTick);
    transport.setActivePattern(1);
    audioBlock(0);

    EXPECT_EQ(transport.songPlayheadTick(), 3840 + 960); // the song keeps playing
    EXPECT_FALSE(transport.playheadTick().has_value());
}

TEST_F(TransportPresenterSongTest, FTR05_SwitchingTheModeRestartsFromTheStart)
{
    transport.play();
    audioBlock(1000 * kTick);
    EXPECT_FALSE(transport.songPlayheadTick().has_value());

    transport.setPlayMode(PlayMode::song);
    audioBlock(0);
    EXPECT_TRUE(transport.isPlaying());
    EXPECT_EQ(transport.songPlayheadTick(), 0);

    transport.setPlayMode(PlayMode::song);
    audioBlock(1000 * kTick);
    EXPECT_EQ(transport.songPlayheadTick(), 1000);

    transport.stop();
    audioBlock();
    transport.setPlayMode(PlayMode::pattern);
    audioBlock();
    EXPECT_FALSE(transport.isPlaying());
    EXPECT_EQ(transport.positionText(), "001.1.000");
}

TEST_F(TransportPresenterSongTest, FTR05_SongModeOnlyPlaysEvenWithRecArmed)
{
    transport.setPlayMode(PlayMode::song);
    transport.setCountInBars(0);
    transport.toggleRecordArmed();
    transport.play();
    audioBlockWithHit(0);
    transport.tick();

    EXPECT_TRUE(transport.isPlaying());
    EXPECT_FALSE(transport.isRecording());
    EXPECT_EQ(pattern().numNotes(), 0);
}

TEST_F(TransportPresenterSongTest, FTR05_TheModeCannotChangeDuringARecording)
{
    transport.setCountInBars(0);
    transport.toggleRecordArmed();
    transport.play();
    audioBlock();

    transport.setPlayMode(PlayMode::song);
    audioBlock();

    EXPECT_EQ(transport.playMode(), PlayMode::pattern);
    EXPECT_TRUE(transport.isRecording());
    transport.stop();
}

TEST_F(TransportPresenterTest, FTR07_PlayWithoutRecArmedOnlyPlays)
{
    transport.play();
    audioBlockWithHit(0);
    transport.tick();
    EXPECT_FALSE(transport.isRecording());
    EXPECT_EQ(pattern().numNotes(), 0);
}

TEST_F(TransportPresenterTest, FTR07_FTR09_FIN07_RecordsAfterTheCountInUntilStop)
{
    transport.toggleRecordArmed();
    EXPECT_TRUE(transport.isRecordArmed());
    transport.play();
    EXPECT_FALSE(transport.isRecording());
    audioBlock();
    EXPECT_TRUE(transport.isRecording());
    EXPECT_TRUE(transport.isCountingIn());
    audioBlock(kBar - 480);
    EXPECT_FALSE(transport.isCountingIn());

    audioBlock();
    audioBlockWithHit(3); // block starts at tick 19.2
    transport.tick();
    ASSERT_EQ(pattern().numNotes(), 1);
    EXPECT_EQ(pattern().note(0).startTick(), 19);
    EXPECT_EQ(pattern().note(0).origin(), model::NoteOrigin::live);

    transport.stop();
    EXPECT_FALSE(transport.isRecording());
    EXPECT_TRUE(transport.isRecordArmed());
    ASSERT_TRUE(undoManager.undo());
    EXPECT_EQ(pattern().numNotes(), 0);
}

TEST_F(TransportPresenterTest, FIN09_EachRunIsItsOwnUndoStep)
{
    transport.toggleRecordArmed();
    transport.setCountInBars(0);
    for (int run = 0; run < 2; ++run)
    {
        transport.play();
        audioBlock();
        audioBlockWithHit(run);
        transport.tick();
        transport.stop();
        audioBlock();
    }
    EXPECT_EQ(pattern().numNotes(), 2);
    ASSERT_TRUE(undoManager.undo());
    EXPECT_EQ(pattern().numNotes(), 1);
}

TEST_F(TransportPresenterTest, FIN09_ReplaceModeStartsFromAnEmptyPattern)
{
    pattern().addNote({.slotNote = 36, .startTick = 0});
    transport.setRecordMode(model::RecordMode::replace);
    EXPECT_EQ(transport.recordMode(), model::RecordMode::replace);
    transport.toggleRecordArmed();
    transport.play();
    EXPECT_EQ(pattern().numNotes(), 0);
}

TEST_F(TransportPresenterTest, FIN07_RecordingEndsWithThePatternWithoutLoop)
{
    transport.setLoop(false);
    transport.setCountInBars(0);
    transport.toggleRecordArmed();
    transport.play();
    audioBlock();
    transport.tick();
    audioBlock(2 * kBar - 960);
    audioBlockWithHit(1);
    EXPECT_FALSE(transport.isPlaying());
    transport.tick();
    EXPECT_FALSE(transport.isRecording());
    EXPECT_FALSE(recorder.isRecording());
    EXPECT_EQ(pattern().numNotes(), 1);
}

TEST_F(TransportPresenterTest, FIN07_HitsOfAStoppedRecordingAreDropped)
{
    transport.toggleRecordArmed();
    transport.setCountInBars(0);
    transport.play();
    audioBlock();
    transport.stop();
    // The audio thread still records until it sees the stop.
    const std::array hits{engine::LiveHit{0, 100, 50.0}};
    sequencer.record(hits, 50.0);
    transport.tick();
    EXPECT_EQ(pattern().numNotes(), 0);

    sequencer.record(hits, 50.0);
    audioBlock();
    transport.play(); // the next recording must not take them either
    transport.tick();
    EXPECT_EQ(pattern().numNotes(), 0);
    EXPECT_TRUE(recorder.isRecording()); // waits for the audio thread to start it
}

TEST_F(TransportPresenterTest, FIN07_PlayWhilePlayingStartsNoRecording)
{
    transport.play();
    audioBlock();
    transport.toggleRecordArmed();
    transport.play();
    audioBlock();
    EXPECT_FALSE(transport.isRecording());
    EXPECT_FALSE(recorder.isRecording());
}

TEST_F(TransportPresenterTest, FIN07_WithoutPatternItOnlyPlays)
{
    transport.setActivePattern(4);
    EXPECT_EQ(transport.activePattern(), 4);
    transport.toggleRecordArmed();
    transport.play();
    audioBlock();
    EXPECT_TRUE(transport.isPlaying());
    EXPECT_FALSE(transport.isRecording());
    transport.stop();
}

TEST_F(TransportPresenterTest, FTR09_CountInIsZeroToTwoBars)
{
    EXPECT_EQ(transport.countInBars(), 1);
    transport.setCountInBars(3);
    EXPECT_EQ(transport.countInBars(), 2);
    transport.setCountInBars(-1);
    EXPECT_EQ(transport.countInBars(), 0);
}

TEST_F(TransportPresenterTest, FTR08_MetronomeSettingsReachTheEngine)
{
    EXPECT_FALSE(transport.metronomeOnPlayback());
    EXPECT_TRUE(transport.metronomeOnRecord());
    transport.setMetronomeOnPlayback(true);
    transport.setMetronomeOnRecord(false);
    EXPECT_TRUE(transport.metronomeOnPlayback());
    EXPECT_FALSE(transport.metronomeOnRecord());
    transport.play();
    audioBlock();
    EXPECT_EQ(block.numClicks, 1U);

    transport.setMetronomeLevel(0.25F);
    EXPECT_FLOAT_EQ(transport.metronomeLevel(), 0.25F);
    EXPECT_FLOAT_EQ(metronome.level(), 0.25F);
}

TEST_F(TransportPresenterTest, FIN08_OutputLatencyAndOffsetMoveRecordedHits)
{
    transport.setOutputLatencyMs(10.0);
    transport.setRecordOffsetMs(5.0); // 5 ms later: 5 ms = 9.6 ticks back in total
    transport.setCountInBars(0);
    transport.toggleRecordArmed();
    transport.play();
    audioBlock();
    audioBlockWithHit(0); // block start tick 19.2
    transport.tick();
    ASSERT_EQ(pattern().numNotes(), 1);
    EXPECT_EQ(pattern().note(0).startTick(), 10);
}

TEST_F(TransportPresenterTest, FIN08_RecordOffsetIsClampedAndSaved)
{
    ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(true));
    EXPECT_CALL(fileSystem, writeText(_, std::string{"50"})).WillOnce(Return(true));
    transport.setRecordOffsetMs(70.0);
    EXPECT_DOUBLE_EQ(transport.recordOffsetMs(), 50.0);
}

TEST_F(TransportPresenterTest, FIN08_RecordOffsetIsLoadedFromTheSettings)
{
    for (const auto& [saved, expected] : {std::pair{"-12.5", -12.5}, {"99", 50.0}, {"abc", 0.0}})
    {
        NiceMock<io::MockFileSystem> files;
        ON_CALL(files, readText(_)).WillByDefault(Return(std::string{saved}));
        io::SettingsStore store{files, "recording-offset.txt"};
        const TransportPresenter loaded{tree, sequencer, metronome, recorder, store};
        EXPECT_DOUBLE_EQ(loaded.recordOffsetMs(), expected) << saved;
    }
}

} // namespace
} // namespace drumprog::ui
