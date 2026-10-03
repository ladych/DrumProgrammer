#include "ui/KitPresenter.h"

#include "engine/MockSampleLoader.h"
#include "engine/RenderHelpers.h"
#include "engine/VelocityCurve.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace drumprog::ui
{
namespace
{

using engine::KitDescription;
using engine::KitSlotDescription;
using engine::SampleBuffer;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

const SampleBuffer kSnare{48000.0, {std::vector<float>(1000, 1.0F)}};

KitDescription smallKit()
{
    return {KitSlotDescription{.gmNote = 36, .midiNote = 36, .name = "Kick", .sampleFile = "/kits/kick.wav"},
            KitSlotDescription{.gmNote = 38, .midiNote = 38, .name = "Snare"},
            KitSlotDescription{.gmNote = 58, .midiNote = 58, .name = "Vibraslap", .coreSlot = false}};
}

class KitPresenterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(loader, load(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(loader, load(std::filesystem::path{"/kits/snare.wav"})).WillByDefault(Return(kSnare));
        engine.prepare(48000.0);
    }

    float renderFrame()
    {
        engine::StereoOutput out(1);
        engine.render(out.channels.data(), 2, 1);
        return out.left[0];
    }

    NiceMock<engine::MockSampleLoader> loader;
    engine::KitBuilder builder{loader};
    engine::SampleEngine engine;
    KitPresenter presenter{smallKit(), builder, engine};
};

TEST_F(KitPresenterTest, FSE04_ShowsCoreSlotsUnlessExpanded)
{
    EXPECT_EQ(presenter.visibleSlots(false), (std::vector<int>{0, 1}));
    EXPECT_EQ(presenter.visibleSlots(true), (std::vector<int>{0, 1, 2}));
}

TEST_F(KitPresenterTest, FPJ03_ReportsMissingSampleOnRateChange)
{
    presenter.setDeviceSampleRate(44100.0);
    ASSERT_EQ(presenter.missingSamples().size(), 1U);
    EXPECT_EQ(presenter.missingSamples()[0], std::filesystem::path{"/kits/kick.wav"});
}

TEST_F(KitPresenterTest, FSE04_LabelsSampleFileOrNoSample)
{
    EXPECT_EQ(presenter.sampleLabel(0), "kick.wav");
    EXPECT_EQ(presenter.sampleLabel(1), "kein Sample");
    EXPECT_EQ(presenter.sampleLabel(7), "");
}

TEST_F(KitPresenterTest, FSE05_NoteLabelShowsGmDefaultUntilOverridden)
{
    EXPECT_EQ(presenter.noteLabel(1), "38 (GM-Default)");
    presenter.select(1);
    presenter.setMidiNote(40);
    EXPECT_EQ(presenter.noteLabel(1), "40");
    EXPECT_EQ(presenter.noteLabel(-1), "");
}

TEST_F(KitPresenterTest, FSE05_OverriddenNoteTriggersSlot)
{
    presenter.select(1);
    ASSERT_TRUE(presenter.loadSample("/kits/snare.wav"));
    presenter.setMidiNote(40);
    engine.queueTrigger(40, 127);
    EXPECT_FLOAT_EQ(renderFrame(), 1.0F);
}

TEST_F(KitPresenterTest, FSE05_ClampsMidiNote)
{
    presenter.select(1);
    presenter.setMidiNote(200);
    EXPECT_EQ(presenter.kit()[1].midiNote, 127);
    presenter.setMidiNote(-3);
    EXPECT_EQ(presenter.kit()[1].midiNote, 0);
}

TEST_F(KitPresenterTest, FSE10_SelectionIgnoresInvalidIndex)
{
    EXPECT_FALSE(presenter.selectedSlot().has_value());
    presenter.select(1);
    EXPECT_EQ(presenter.selectedSlot(), 1);
    presenter.select(3);
    presenter.select(-1);
    EXPECT_EQ(presenter.selectedSlot(), 1);
}

TEST_F(KitPresenterTest, FSE10_EditsWithoutSelectionDoNothing)
{
    EXPECT_FALSE(presenter.loadSample("/kits/snare.wav"));
    presenter.setGainDb(-6.0F);
    presenter.setPitch(3);
    presenter.setMidiNote(40);
    EXPECT_FALSE(presenter.previewSelected());
    EXPECT_FLOAT_EQ(presenter.gainDb(), 0.0F);
    EXPECT_EQ(presenter.kit()[1].midiNote, 38);
}

TEST_F(KitPresenterTest, FSE01_LoadsSampleIntoSelectedSlot)
{
    presenter.select(1);
    EXPECT_TRUE(presenter.loadSample("/kits/snare.wav"));
    EXPECT_EQ(presenter.sampleLabel(1), "snare.wav");
    EXPECT_TRUE(presenter.previewSelected());
    EXPECT_FLOAT_EQ(renderFrame(), engine::velocityToGain(engine::SampleEngine::kPreviewVelocity));
}

TEST_F(KitPresenterTest, FPJ03_FailedLoadKeepsPreviousSample)
{
    presenter.select(1);
    ASSERT_TRUE(presenter.loadSample("/kits/snare.wav"));
    EXPECT_FALSE(presenter.loadSample("/kits/broken.wav"));
    EXPECT_EQ(presenter.sampleLabel(1), "snare.wav");
    EXPECT_TRUE(presenter.missingSamples().empty() ||
                std::find(presenter.missingSamples().begin(), presenter.missingSamples().end(),
                          std::filesystem::path{"/kits/broken.wav"}) == presenter.missingSamples().end());
}

TEST_F(KitPresenterTest, FSE06_GainIsSetInDecibelsAndClamped)
{
    presenter.select(1);
    presenter.setGainDb(-6.0F);
    EXPECT_NEAR(presenter.gainDb(), -6.0F, 1.0e-4);
    EXPECT_NEAR(presenter.kit()[1].gain, std::pow(10.0F, -6.0F / 20.0F), 1.0e-6);
    presenter.setGainDb(20.0F);
    EXPECT_NEAR(presenter.gainDb(), KitPresenter::kMaxGainDb, 1.0e-4);
    presenter.setGainDb(-100.0F);
    EXPECT_NEAR(presenter.gainDb(), KitPresenter::kMinGainDb, 1.0e-4);
}

TEST_F(KitPresenterTest, FSE06_GainDbOfSilentSlotIsMinimum)
{
    KitDescription kit{KitSlotDescription{.midiNote = 36, .gain = 0.0F}};
    KitPresenter silent{kit, builder, engine};
    silent.select(0);
    EXPECT_FLOAT_EQ(silent.gainDb(), KitPresenter::kMinGainDb);
}

TEST_F(KitPresenterTest, FSE06_GainChangeReachesEngine)
{
    presenter.select(1);
    presenter.loadSample("/kits/snare.wav");
    presenter.setGainDb(-6.0F);
    engine.queueTrigger(38, 127);
    EXPECT_NEAR(renderFrame(), std::pow(10.0F, -6.0F / 20.0F), 1.0e-5);
}

TEST_F(KitPresenterTest, FSE07_PitchIsClampedToTwelveSemitones)
{
    presenter.select(1);
    presenter.setPitch(5);
    EXPECT_EQ(presenter.kit()[1].pitchSemitones, 5);
    presenter.setPitch(30);
    EXPECT_EQ(presenter.kit()[1].pitchSemitones, 12);
    presenter.setPitch(-30);
    EXPECT_EQ(presenter.kit()[1].pitchSemitones, -12);
}

TEST_F(KitPresenterTest, FSE09_LedLightsAfterHitAndGoesOffAfterHoldTime)
{
    presenter.select(1);
    presenter.loadSample("/kits/snare.wav");
    EXPECT_FALSE(presenter.isLedOn(1));
    presenter.previewSelected();
    renderFrame();
    presenter.tick();
    EXPECT_TRUE(presenter.isLedOn(1));
    EXPECT_FALSE(presenter.isLedOn(0));
    for (int i = 1; i < KitPresenter::kLedHoldTicks; ++i)
        presenter.tick();
    EXPECT_TRUE(presenter.isLedOn(1));
    presenter.tick();
    EXPECT_FALSE(presenter.isLedOn(1));
    EXPECT_FALSE(presenter.isLedOn(-1));
    EXPECT_FALSE(presenter.isLedOn(500));
}

} // namespace
} // namespace drumprog::ui
