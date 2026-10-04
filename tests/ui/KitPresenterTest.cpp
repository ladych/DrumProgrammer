#include "ui/KitPresenter.h"

#include "engine/MockSampleLoader.h"
#include "engine/RenderHelpers.h"
#include "engine/VelocityCurve.h"
#include "model/FakeIdGenerator.h"
#include "model/ModelIds.h"
#include "model/ProjectFactory.h"
#include "model/SnapshotPublisher.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace drumprog::ui
{
namespace
{

using engine::SampleBuffer;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

const SampleBuffer kSnareSample{48000.0, {std::vector<float>(1000, 1.0F)}};

constexpr int kKick = 1;       // GM 36
constexpr int kSnare = 3;      // GM 38
constexpr int kVibraslap = 23; // GM 58, not a core slot

class KitPresenterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(loader, load(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(loader, load(std::filesystem::path{"/kits/snare.wav"})).WillByDefault(Return(kSnareSample));
        engine.prepare(48000.0);
    }

    float renderFrame()
    {
        engine::StereoOutput out(1);
        engine.render(out.channels.data(), 2, 1);
        return out.left[0];
    }

    [[nodiscard]] model::SampleSlot slot(int index) const { return project.kit().slot(index); }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    model::Project project{tree, nullptr};
    juce::UndoManager undoManager;
    NiceMock<engine::MockSampleLoader> loader;
    engine::KitBuilder builder{loader};
    engine::SampleEngine engine;
    engine::KitPublisher kits{builder, engine};
    model::ProjectSnapshotExchange snapshots;
    model::SnapshotPublisher publisher{tree, snapshots, kits};
    KitPresenter presenter{tree, undoManager, kits, engine};
};

TEST_F(KitPresenterTest, FSE04_ShowsCoreSlotsUnlessExpanded)
{
    const auto core = presenter.visibleSlots(false);
    EXPECT_EQ(presenter.visibleSlots(true).size(), 25U);
    EXPECT_EQ(core.size(), 19U);
    EXPECT_EQ(std::count(core.begin(), core.end(), kVibraslap), 0);
    EXPECT_EQ(std::count(core.begin(), core.end(), kSnare), 1);
}

TEST_F(KitPresenterTest, FSE04_NamesAndNotesComeFromTheModel)
{
    EXPECT_EQ(presenter.numSlots(), 25);
    EXPECT_EQ(presenter.slotName(kSnare), "Acoustic Snare");
    EXPECT_EQ(presenter.midiNote(kSnare), 38);
    EXPECT_EQ(presenter.slotName(25), "");
    EXPECT_EQ(presenter.midiNote(-1), -1);
}

TEST_F(KitPresenterTest, FSE04_LabelsSampleFileOrNoSample)
{
    slot(kKick).setFilePath("/kits/kick.wav");

    EXPECT_EQ(presenter.sampleLabel(kKick), "kick.wav");
    EXPECT_EQ(presenter.sampleLabel(kSnare), "kein Sample");
    EXPECT_EQ(presenter.sampleLabel(25), "");
    EXPECT_EQ(presenter.sampleLabel(-1), "");
}

TEST_F(KitPresenterTest, FPJ03_SampleMissingOnLoadIsShownAsNoSample)
{
    slot(kKick).setFilePath("/kits/kick.wav");
    juce::ValueTree{slot(kKick).tree()}.setProperty(model::ids::sampleMissing, true, nullptr);

    EXPECT_EQ(presenter.sampleLabel(kKick), "kein Sample");
}

TEST_F(KitPresenterTest, FPJ03_ReportsSamplesTheEngineCouldNotLoad)
{
    slot(kKick).setFilePath("/kits/kick.wav");

    ASSERT_EQ(presenter.missingSamples().size(), 1U);
    EXPECT_EQ(presenter.missingSamples()[0], std::filesystem::path{"/kits/kick.wav"});
}

TEST_F(KitPresenterTest, FSE05_NoteLabelShowsGmDefaultUntilOverridden)
{
    EXPECT_EQ(presenter.noteLabel(kSnare), "38 (GM-Default)");
    presenter.select(kSnare);
    presenter.setMidiNote(40);
    EXPECT_EQ(presenter.noteLabel(kSnare), "40");
    EXPECT_EQ(presenter.noteLabel(-1), "");
    EXPECT_EQ(presenter.noteLabel(25), "");
}

TEST_F(KitPresenterTest, FSE05_OverriddenNoteTriggersSlot)
{
    presenter.select(kSnare);
    ASSERT_TRUE(presenter.loadSample("/kits/snare.wav"));
    presenter.setMidiNote(40);
    engine.queueTrigger(40, 127);
    EXPECT_FLOAT_EQ(renderFrame(), 1.0F);
}

TEST_F(KitPresenterTest, FSE05_ClampsMidiNote)
{
    presenter.select(kSnare);
    presenter.setMidiNote(200);
    EXPECT_EQ(slot(kSnare).midiNote(), 127);
    presenter.setMidiNote(-3);
    EXPECT_EQ(slot(kSnare).midiNote(), 0);
}

TEST_F(KitPresenterTest, FSE10_SelectionIgnoresInvalidIndex)
{
    EXPECT_FALSE(presenter.selectedSlot().has_value());
    presenter.select(kSnare);
    EXPECT_EQ(presenter.selectedSlot(), kSnare);
    presenter.select(25);
    presenter.select(-1);
    EXPECT_EQ(presenter.selectedSlot(), kSnare);
}

TEST_F(KitPresenterTest, FSE10_EditsWithoutSelectionDoNothing)
{
    EXPECT_FALSE(presenter.loadSample("/kits/snare.wav"));
    presenter.setGainDb(-6.0F);
    presenter.setPitch(3);
    presenter.setMidiNote(40);
    EXPECT_FALSE(presenter.previewSelected());
    EXPECT_FLOAT_EQ(presenter.gainDb(), 0.0F);
    EXPECT_EQ(presenter.pitch(), 0);
    EXPECT_FALSE(undoManager.canUndo());
}

TEST_F(KitPresenterTest, FSE01_LoadsSampleIntoSelectedSlot)
{
    presenter.select(kSnare);
    EXPECT_TRUE(presenter.loadSample("/kits/snare.wav"));
    EXPECT_EQ(presenter.sampleLabel(kSnare), "snare.wav");
    EXPECT_EQ(slot(kSnare).filePath(), "/kits/snare.wav");
    EXPECT_TRUE(presenter.previewSelected());
    EXPECT_FLOAT_EQ(renderFrame(), engine::velocityToGain(engine::SampleEngine::kPreviewVelocity));
}

TEST_F(KitPresenterTest, FSE10_PreviewReportsFullTriggerQueue)
{
    presenter.select(kSnare);
    for (std::size_t i = 0; i < engine::SampleEngine::kTriggerQueueSize; ++i)
        ASSERT_TRUE(presenter.previewSelected());
    EXPECT_FALSE(presenter.previewSelected());
}

TEST_F(KitPresenterTest, FPJ03_FailedLoadKeepsPreviousSample)
{
    presenter.select(kSnare);
    ASSERT_TRUE(presenter.loadSample("/kits/snare.wav"));
    EXPECT_FALSE(presenter.loadSample("/kits/broken.wav"));
    EXPECT_EQ(presenter.sampleLabel(kSnare), "snare.wav");
    EXPECT_TRUE(presenter.missingSamples().empty());
}

TEST_F(KitPresenterTest, FSE06_GainIsSetInDecibelsAndClamped)
{
    presenter.select(kSnare);
    presenter.setGainDb(-6.0F);
    EXPECT_NEAR(presenter.gainDb(), -6.0F, 1.0e-4);
    EXPECT_NEAR(slot(kSnare).gain(), std::pow(10.0, -6.0 / 20.0), 1.0e-6);
    presenter.setGainDb(20.0F);
    EXPECT_NEAR(presenter.gainDb(), KitPresenter::kMaxGainDb, 1.0e-4);
    presenter.setGainDb(-100.0F);
    EXPECT_NEAR(presenter.gainDb(), KitPresenter::kMinGainDb, 1.0e-4);
}

TEST_F(KitPresenterTest, FSE06_GainDbOfSilentSlotIsMinimum)
{
    slot(kSnare).setGain(0.0);
    presenter.select(kSnare);
    EXPECT_FLOAT_EQ(presenter.gainDb(), KitPresenter::kMinGainDb);
}

TEST_F(KitPresenterTest, FSE06_GainChangeReachesEngine)
{
    presenter.select(kSnare);
    presenter.loadSample("/kits/snare.wav");
    presenter.setGainDb(-6.0F);
    engine.queueTrigger(38, 127);
    EXPECT_NEAR(renderFrame(), std::pow(10.0F, -6.0F / 20.0F), 1.0e-5);
}

TEST_F(KitPresenterTest, FSE07_PitchIsClampedToTwelveSemitones)
{
    presenter.select(kSnare);
    presenter.setPitch(5);
    EXPECT_EQ(presenter.pitch(), 5);
    presenter.setPitch(30);
    EXPECT_EQ(presenter.pitch(), 12);
    presenter.setPitch(-30);
    EXPECT_EQ(slot(kSnare).pitch(), -12.0);
}

TEST_F(KitPresenterTest, FPJ05_SlotEditsAreUndoable)
{
    presenter.select(kSnare);
    presenter.setMidiNote(40);
    presenter.setPitch(3);

    undoManager.undo();
    EXPECT_EQ(presenter.pitch(), 0);
    EXPECT_EQ(slot(kSnare).midiNote(), 40);
    undoManager.undo();
    EXPECT_EQ(slot(kSnare).midiNote(), 38);
}

TEST_F(KitPresenterTest, FPJ05_DraggingASliderIsOneUndoStep)
{
    presenter.select(kSnare);
    for (float gainDb = -1.0F; gainDb > -10.0F; gainDb -= 1.0F)
        presenter.setGainDb(gainDb);

    undoManager.undo();

    EXPECT_FLOAT_EQ(presenter.gainDb(), 0.0F);
    EXPECT_FALSE(undoManager.canUndo());
}

TEST_F(KitPresenterTest, FPJ05_TheSameEditOnAnotherSlotIsANewUndoStep)
{
    presenter.select(kSnare);
    presenter.setPitch(3);
    presenter.select(kKick);
    presenter.setPitch(4);

    undoManager.undo();

    EXPECT_EQ(slot(kKick).pitch(), 0.0);
    EXPECT_EQ(slot(kSnare).pitch(), 3.0);
}

TEST_F(KitPresenterTest, FPJ02_ChangeCountFollowsEveryProjectChange)
{
    const auto before = presenter.changeCount();
    project.setBpm(90.0);
    EXPECT_NE(presenter.changeCount(), before);
}

TEST_F(KitPresenterTest, FSE09_LedLightsAfterHitAndGoesOffAfterHoldTime)
{
    presenter.select(kSnare);
    presenter.loadSample("/kits/snare.wav");
    EXPECT_FALSE(presenter.isLedOn(kSnare));
    presenter.previewSelected();
    renderFrame();
    presenter.tick();
    EXPECT_TRUE(presenter.isLedOn(kSnare));
    EXPECT_FALSE(presenter.isLedOn(kKick));
    for (int i = 0; i < ActivityLed::kHoldTicks; ++i)
        presenter.tick();
    EXPECT_FALSE(presenter.isLedOn(kSnare));
    EXPECT_FALSE(presenter.isLedOn(-1));
    EXPECT_FALSE(presenter.isLedOn(500));
}

} // namespace
} // namespace drumprog::ui
