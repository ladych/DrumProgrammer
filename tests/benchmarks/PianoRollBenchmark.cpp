#include "app/PianoRollView.h"
#include "app/Theme.h"
#include "engine/Metronome.h"
#include "engine/Sequencer.h"
#include "io/SettingsStore.h"
#include "io/StdFileSystem.h"
#include "model/FakeIdGenerator.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "model/SongTakeRecorder.h"
#include "model/TakeRecorder.h"
#include "ui/ActivePattern.h"
#include "ui/PatternListPresenter.h"
#include "ui/PianoRollPresenter.h"
#include "ui/TransportPresenter.h"

#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>

namespace drumprog::app
{
namespace
{

constexpr int kNotes = 2000;
constexpr int kFrames = 120;
/// Q-07: at least 30 frames per second while scrolling.
constexpr double kMinFramesPerSecond = 30.0;

/// Q-07: the piano roll of the program with a pattern of 2,000 notes, painted the way a repaint does while
/// the horizontal scroll bar moves through the pattern. Every frame paints the whole view (header, rows,
/// notes, velocity lane) into an image with JUCE's software renderer, which is more than a scroll repaints.
TEST(PianoRollBenchmark, Q07_ScrollingTwoThousandNotesKeepsThirtyFramesPerSecond)
{
#ifndef NDEBUG
    GTEST_SKIP() << "only meaningful in an optimised build";
#endif
    const juce::ScopedJuceInitialiser_GUI juce;
    theme::LookAndFeel lookAndFeel;
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
    {
        model::FakeIdGenerator ids;
        juce::UndoManager undoManager;
        juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
        juce::ValueTree globalKit = model::ProjectFactory::createDefaultKit();
        model::Project project{tree, nullptr};
        // 16 rows of 125 notes of a 1/32 each, 16 bars: dense where the view starts, and long to scroll.
        auto pattern = project.pattern(0);
        pattern.setLengthBars(16);
        const int gmNotes[] = {36, 38, 40, 42, 44, 46, 49, 51, 37, 39, 41, 43, 45, 47, 48, 50};
        for (int index = 0; index < kNotes; ++index)
            pattern.addNote({gmNotes[index % 16], (index / 16) * 240LL, 120, 1 + index % 127});

        io::StdFileSystem fileSystem;
        io::SettingsStore offsetSettings{
            fileSystem, std::filesystem::temp_directory_path() / "drumprog_benchmark_offset.txt"};
        engine::Sequencer sequencer;
        engine::Metronome metronome;
        model::TakeRecorder takeRecorder{tree, globalKit, undoManager};
        model::SongTakeRecorder songTakeRecorder{tree, globalKit, undoManager, ids};
        ui::ActivePattern active{tree};
        ui::PatternListPresenter patterns{tree, undoManager, ids, active};
        ui::PianoRollPresenter presenter{
            tree, globalKit, undoManager, active, [](int) { return std::string{}; }};
        ui::TransportPresenter transport{
            tree, sequencer, metronome, takeRecorder, songTakeRecorder, offsetSettings};
        ASSERT_EQ(presenter.notes().size(), static_cast<std::size_t>(kNotes));

        PianoRollView view{presenter, transport, patterns};
        view.setSize(1400, 700);
        juce::ScrollBar* horizontal = nullptr;
        for (auto* child : view.getChildren())
            if (auto* bar = dynamic_cast<juce::ScrollBar*>(child); bar != nullptr && !bar->isVertical())
                horizontal = bar;
        ASSERT_NE(horizontal, nullptr);
        const auto range = horizontal->getMaximumRangeLimit() - horizontal->getCurrentRangeSize();
        ASSERT_GT(range, 0.0);

        double slowestMs = 0.0;
        const auto start = std::chrono::steady_clock::now();
        for (int frame = 0; frame < kFrames; ++frame)
        {
            horizontal->setCurrentRangeStart(range * frame / (kFrames - 1));
            horizontal->handleUpdateNowIfNeeded();
            const auto frameStart = std::chrono::steady_clock::now();
            const auto image = view.createComponentSnapshot(view.getLocalBounds());
            const std::chrono::duration<double, std::milli> frameTime =
                std::chrono::steady_clock::now() - frameStart;
            slowestMs = std::max(slowestMs, frameTime.count());
            ASSERT_TRUE(image.isValid());
        }
        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

        const double framesPerSecond = kFrames / elapsed.count();
        std::printf("Q-07: %d notes: %.0f frames per second, slowest frame %.1f ms\n",
                    kNotes,
                    framesPerSecond,
                    slowestMs);
        ::testing::Test::RecordProperty("frames_per_second", std::to_string(framesPerSecond));
        EXPECT_GE(framesPerSecond, kMinFramesPerSecond);
    }
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

} // namespace
} // namespace drumprog::app
