#include "ui/KitPresenter.h"

#include "engine/MockDrumGizmoReader.h"
#include "engine/MockSampleLoader.h"
#include "engine/RenderHelpers.h"
#include "engine/VelocityCurve.h"
#include "io/Utf8Path.h"
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

    /// Slot of the active kit: the global kit unless the project has its own.
    [[nodiscard]] model::SampleSlot slot(int index) const { return project.activeKit(globalKit).slot(index); }
    [[nodiscard]] model::Kit global() const { return {globalKit, nullptr}; }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    juce::ValueTree globalKit = model::ProjectFactory::createDefaultKit();
    model::Project project{tree, nullptr};
    juce::UndoManager undoManager;
    NiceMock<engine::MockSampleLoader> loader;
    engine::KitBuilder builder{loader};
    engine::SampleEngine engine;
    engine::KitPublisher kits{builder, engine};
    model::ProjectSnapshotExchange snapshots;
    model::SnapshotPublisher publisher{tree, globalKit, snapshots, kits};
    NiceMock<engine::MockDrumGizmoReader> drumGizmoReader;
    KitPresenter presenter{tree, globalKit, undoManager, kits, engine, drumGizmoReader};
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

TEST_F(KitPresenterTest, FPJ02_ChangeCountFollowsEveryGlobalKitChange)
{
    const auto before = presenter.changeCount();
    global().slot(kSnare).setGain(0.5);
    EXPECT_NE(presenter.changeCount(), before);
}

TEST_F(KitPresenterTest, FSE05_ANewProjectEditsTheGlobalKit)
{
    EXPECT_FALSE(presenter.usesProjectKit());
    EXPECT_EQ(presenter.kitSourceLabel(), "Programm-Kit");

    presenter.select(kSnare);
    presenter.setPitch(3);

    EXPECT_EQ(global().slot(kSnare).pitch(), 3.0);
    EXPECT_FALSE(project.hasOwnKit());
}

TEST_F(KitPresenterTest, FSE05_CopyingTheKitIntoTheProjectDecouplesItFromTheGlobalKit)
{
    global().slot(kSnare).setPitch(2.0);

    presenter.copyGlobalKitToProject();
    presenter.select(kSnare);
    presenter.setPitch(5);

    EXPECT_TRUE(presenter.usesProjectKit());
    EXPECT_EQ(presenter.kitSourceLabel(), "Projekt-Kit");
    EXPECT_EQ(project.kit().slot(kSnare).pitch(), 5.0);
    EXPECT_EQ(global().slot(kSnare).pitch(), 2.0);
}

TEST_F(KitPresenterTest, FSE05_TheProjectKitIsWhatTheEngineHears)
{
    presenter.copyGlobalKitToProject();
    presenter.select(kSnare);
    ASSERT_TRUE(presenter.loadSample("/kits/snare.wav"));

    engine.queueTrigger(38, 127);
    EXPECT_FLOAT_EQ(renderFrame(), 1.0F);
    EXPECT_FALSE(global().slot(kSnare).hasSample());
}

TEST_F(KitPresenterTest, FSE05_UsingTheGlobalKitAgainRemovesTheProjectKit)
{
    presenter.copyGlobalKitToProject();
    project.kit().slot(kSnare).setPitch(5.0);

    presenter.useGlobalKit();

    EXPECT_FALSE(presenter.usesProjectKit());
    EXPECT_EQ(slot(kSnare).pitch(), 0.0);
}

TEST_F(KitPresenterTest, FSE05_TheProjectKitCanBecomeTheGlobalKit)
{
    presenter.copyGlobalKitToProject();
    project.kit().slot(kSnare).setFilePath("/kits/snare.wav");

    presenter.copyProjectKitToGlobal();

    EXPECT_EQ(global().slot(kSnare).filePath(), "/kits/snare.wav");
    EXPECT_TRUE(presenter.usesProjectKit());
}

TEST_F(KitPresenterTest, FSE05_KitSourceCommandsThatMakeNoSenseDoNothing)
{
    presenter.useGlobalKit();
    presenter.copyProjectKitToGlobal();
    EXPECT_FALSE(undoManager.canUndo());

    presenter.copyGlobalKitToProject();
    global().slot(kSnare).setPitch(2.0);
    presenter.copyGlobalKitToProject();
    EXPECT_EQ(project.kit().slot(kSnare).pitch(), 0.0);
}

TEST_F(KitPresenterTest, FPJ05_KitSourceChangesAreUndoSteps)
{
    presenter.copyGlobalKitToProject();
    project.kit().slot(kSnare).setPitch(5.0);
    presenter.copyProjectKitToGlobal();
    presenter.useGlobalKit();

    undoManager.undo();
    EXPECT_TRUE(presenter.usesProjectKit());
    EXPECT_EQ(global().slot(kSnare).pitch(), 5.0);
    undoManager.undo();
    EXPECT_EQ(global().slot(kSnare).pitch(), 0.0);
    undoManager.undo();
    EXPECT_FALSE(presenter.usesProjectKit());
}

TEST_F(KitPresenterTest, FSE04_ResetRestoresTheDefaultKitAsOneUndoStep)
{
    auto defaultKit = model::ProjectFactory::createDefaultKit();
    model::Kit{defaultKit, nullptr}.slot(kSnare).setFilePath("/kits/snare.wav");
    presenter.select(kSnare);
    presenter.setPitch(4);
    presenter.setMidiNote(60);

    presenter.resetToDefaultKit(defaultKit);

    EXPECT_EQ(global().slot(kSnare).filePath(), "/kits/snare.wav");
    EXPECT_EQ(global().slot(kSnare).pitch(), 0.0);
    EXPECT_EQ(global().slot(kSnare).midiNote(), 38);
    engine.queueTrigger(38, 127);
    EXPECT_FLOAT_EQ(renderFrame(), 1.0F);
    undoManager.undo();
    EXPECT_EQ(global().slot(kSnare).pitch(), 4.0);
    EXPECT_FALSE(global().slot(kSnare).hasSample());
}

TEST_F(KitPresenterTest, FSE05_ResetChangesOnlyTheProjectKitIfTheProjectHasOne)
{
    auto defaultKit = model::ProjectFactory::createDefaultKit();
    model::Kit{defaultKit, nullptr}.slot(kSnare).setFilePath("/kits/snare.wav");
    presenter.copyGlobalKitToProject();

    presenter.resetToDefaultKit(defaultKit);

    EXPECT_EQ(project.kit().slot(kSnare).filePath(), "/kits/snare.wav");
    EXPECT_FALSE(global().slot(kSnare).hasSample());
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

const std::filesystem::path kDrumkit{"/kits/Monster/drumkit.xml"};

engine::DrumGizmoKit testDrumGizmoKit(const std::string& name = "Testkit")
{
    const auto hitOf = [](const char* file)
    {
        return engine::DrumGizmoSample{
            .name = file, .power = 1.0, .audioFiles = {{.channel = "Close", .file = file, .fileChannel = 1}}};
    };
    engine::DrumGizmoKit kit;
    kit.name = name;
    kit.instruments = {{.name = "Kdrum", .group = "", .samples = {hitOf("/kits/Monster/kick.wav")}},
                       {.name = "Snare", .group = "", .samples = {hitOf("/kits/Monster/gone.wav")}},
                       {.name = "Hihat", .group = "hihat", .samples = {}}};
    return kit;
}

class KitPresenterDrumGizmoTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(loader, load(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(loader, load(std::filesystem::path{"/kits/Monster/kick.wav"}))
            .WillByDefault(Return(kSnareSample));
        ON_CALL(reader, read(kDrumkit, _))
            .WillByDefault(Return(engine::DrumGizmoReadResult{.kit = testDrumGizmoKit()}));
        engine.prepare(48000.0);
    }

    void finishLoading()
    {
        background.waitUntilFinished();
        presenter.tick();
    }

    [[nodiscard]] model::Kit global() const { return {globalKit, nullptr}; }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    juce::ValueTree globalKit = model::ProjectFactory::createDefaultKit();
    juce::UndoManager undoManager;
    NiceMock<engine::MockSampleLoader> loader;
    NiceMock<engine::MockDrumGizmoReader> reader;
    engine::KitBuilder builder{loader};
    engine::SampleEngine engine;
    engine::DrumGizmoBackgroundLoader background{reader, loader};
    engine::KitPublisher kits{builder, engine, &background};
    model::ProjectSnapshotExchange snapshots;
    model::SnapshotPublisher publisher{tree, globalKit, snapshots, kits};
    KitPresenter presenter{tree, globalKit, undoManager, kits, engine, reader};
};

TEST_F(KitPresenterDrumGizmoTest, FDG04_LoadingADrumGizmoKitMapsItsInstrumentsToTheGmSlots)
{
    EXPECT_EQ(presenter.loadDrumGizmoKit(kDrumkit), engine::DrumGizmoError::none);

    EXPECT_TRUE(presenter.isDrumGizmoKit());
    EXPECT_EQ(global().drumkitFile(), io::utf8FromPath(kDrumkit));
    EXPECT_EQ(global().findSlot(36)->instrument(), "Kdrum");
    EXPECT_EQ(global().findSlot(46)->chokeGroup(), engine::kHiHatChokeGroup);
    EXPECT_EQ(presenter.sampleLabel(kKick), "Kdrum");
    EXPECT_EQ(presenter.sampleLabel(kVibraslap), "kein Instrument");
    EXPECT_EQ(presenter.instrumentNames(), (std::vector<std::string>{"Kdrum", "Snare", "Hihat"}));
}

TEST_F(KitPresenterDrumGizmoTest, FPJ05_LoadingADrumGizmoKitIsOneUndoStep)
{
    static_cast<void>(presenter.loadDrumGizmoKit(kDrumkit));

    undoManager.undo();

    EXPECT_FALSE(presenter.isDrumGizmoKit());
    EXPECT_EQ(presenter.sampleLabel(kKick), "kein Sample");
}

TEST_F(KitPresenterDrumGizmoTest, Q09_AKitThatCannotBeReadKeepsTheKit)
{
    ON_CALL(reader, read(kDrumkit, false))
        .WillByDefault(Return(engine::DrumGizmoReadResult{
            .kit = std::nullopt, .error = engine::DrumGizmoError::notADrumkit, .errorFile = kDrumkit}));

    EXPECT_EQ(presenter.loadDrumGizmoKit(kDrumkit), engine::DrumGizmoError::notADrumkit);

    EXPECT_FALSE(presenter.isDrumGizmoKit());
    EXPECT_FALSE(undoManager.canUndo());
}

TEST_F(KitPresenterDrumGizmoTest, FDG09_ShowsTheLoadingStateThenTheRamAndMissingFiles)
{
    static_cast<void>(presenter.loadDrumGizmoKit(kDrumkit));
    EXPECT_THAT(presenter.drumGizmoStatus(),
                ::testing::StartsWith("DrumGizmo-Kit „Testkit“ wird geladen..."));
    EXPECT_FALSE(presenter.hasDrumGizmoWarning());
    const auto changes = presenter.changeCount();

    finishLoading();

    EXPECT_NE(presenter.changeCount(), changes);
    EXPECT_EQ(presenter.drumGizmoStatus(),
              "DrumGizmo-Kit „Testkit“: 0 MB im Arbeitsspeicher | 1 Audiodateien fehlen");
    EXPECT_TRUE(presenter.hasDrumGizmoWarning());
    engine.queueTrigger(36, 100);
    engine::StereoOutput out(1);
    engine.render(out.channels.data(), 2, 1);
    EXPECT_GT(out.left[0], 0.0F);
}

TEST_F(KitPresenterDrumGizmoTest, Q09_AFailedLoadShowsItsError)
{
    ON_CALL(reader, read(kDrumkit, true))
        .WillByDefault(Return(engine::DrumGizmoReadResult{.kit = std::nullopt,
                                                          .error = engine::DrumGizmoError::instrumentMissing,
                                                          .errorFile = "/kits/Monster/Kdrum.xml"}));
    static_cast<void>(presenter.loadDrumGizmoKit(kDrumkit));

    finishLoading();

    EXPECT_EQ(presenter.drumGizmoStatus(),
              "Eine Instrument-Datei des DrumGizmo-Kits fehlt: " +
                  io::utf8FromPath("/kits/Monster/Kdrum.xml"));
    EXPECT_TRUE(presenter.hasDrumGizmoWarning());
}

TEST_F(KitPresenterDrumGizmoTest, FDG09_AKitWithoutNameIsNamedByItsFolder)
{
    ON_CALL(reader, read(kDrumkit, _))
        .WillByDefault(Return(engine::DrumGizmoReadResult{.kit = testDrumGizmoKit("")}));
    static_cast<void>(presenter.loadDrumGizmoKit(kDrumkit));

    EXPECT_THAT(presenter.drumGizmoStatus(), ::testing::StartsWith("DrumGizmo-Kit „Monster“"));
}

TEST_F(KitPresenterDrumGizmoTest, Q09_AnUnreadableKitOfASavedKitHasNoInstruments)
{
    globalKit.setProperty(model::ids::drumkitFile, "/elsewhere/Lost/drumkit.xml", nullptr);

    EXPECT_TRUE(presenter.isDrumGizmoKit());
    EXPECT_TRUE(presenter.instrumentNames().empty());
    EXPECT_THAT(presenter.drumGizmoStatus(), ::testing::StartsWith("DrumGizmo-Kit „Lost“"));
}

TEST_F(KitPresenterDrumGizmoTest, FDG07_TheSelectedSlotCanPlayAnotherInstrument)
{
    static_cast<void>(presenter.loadDrumGizmoKit(kDrumkit));
    presenter.setInstrument("Snare");
    EXPECT_EQ(presenter.instrument(), "");

    presenter.select(kKick);
    presenter.setInstrument("Snare");

    EXPECT_EQ(presenter.instrument(), "Snare");
    EXPECT_EQ(global().findSlot(36)->instrument(), "Snare");
    undoManager.undo();
    EXPECT_EQ(presenter.instrument(), "Kdrum");
}

TEST_F(KitPresenterDrumGizmoTest, FDG04_LoadingTheSameKitAgainReadsItsFilesAgain)
{
    EXPECT_CALL(reader, read(_, false)).Times(::testing::AnyNumber());
    EXPECT_CALL(reader, read(kDrumkit, true)).Times(2);

    static_cast<void>(presenter.loadDrumGizmoKit(kDrumkit));
    finishLoading();
    static_cast<void>(presenter.loadDrumGizmoKit(kDrumkit));
    finishLoading();
}

TEST_F(KitPresenterDrumGizmoTest, FDG04_ASampleKitHasNoDrumGizmoStatus)
{
    EXPECT_FALSE(presenter.isDrumGizmoKit());
    EXPECT_EQ(presenter.drumGizmoStatus(), "");
    EXPECT_FALSE(presenter.hasDrumGizmoWarning());
    EXPECT_TRUE(presenter.instrumentNames().empty());
}

TEST(KitPresenterDrumGizmoStatus, FDG10_DescribesEveryStateInGerman)
{
    using State = engine::DrumGizmoStatus::State;
    EXPECT_EQ(KitPresenter::describeDrumGizmoKit("Aasi", {.state = State::none}, 0.0F),
              "DrumGizmo-Kit „Aasi“");
    EXPECT_EQ(KitPresenter::describeDrumGizmoKit("Aasi", {.state = State::loading}, 0.426F),
              "DrumGizmo-Kit „Aasi“ wird geladen... 43 %");
    EXPECT_EQ(
        KitPresenter::describeDrumGizmoKit("Aasi", {.state = State::ready, .bytes = 1'260'000'000ULL}, 1.0F),
        "DrumGizmo-Kit „Aasi“: 1,3 GB im Arbeitsspeicher");
    EXPECT_EQ(
        KitPresenter::describeDrumGizmoKit("Aasi", {.state = State::ready, .bytes = 349'600'000ULL}, 1.0F),
        "DrumGizmo-Kit „Aasi“: 350 MB im Arbeitsspeicher");
    EXPECT_EQ(
        KitPresenter::describeDrumGizmoKit("Aasi", {.state = State::ready, .bytes = 6'000'000'000ULL}, 1.0F),
        "DrumGizmo-Kit „Aasi“: 6,0 GB im Arbeitsspeicher | mehr als 5 GB, der Arbeitsspeicher könnte knapp "
        "werden");
}

TEST(KitPresenterDrumGizmoStatus, FDG10_WarnsAboutFailuresMissingFilesAndLargeKits)
{
    using State = engine::DrumGizmoStatus::State;
    EXPECT_FALSE(KitPresenter::isDrumGizmoWarning({.state = State::ready, .bytes = 1000}));
    EXPECT_TRUE(KitPresenter::isDrumGizmoWarning({.state = State::failed}));
    EXPECT_TRUE(KitPresenter::isDrumGizmoWarning({.state = State::ready, .missingFiles = 2}));
    EXPECT_TRUE(
        KitPresenter::isDrumGizmoWarning({.state = State::ready, .bytes = KitPresenter::kLargeKitBytes + 1}));
}

TEST(KitPresenterDrumGizmoStatus, Q09_NamesEveryErrorInGerman)
{
    using engine::DrumGizmoError;
    const std::filesystem::path file{"x.xml"};
    EXPECT_EQ(KitPresenter::drumGizmoErrorMessage(DrumGizmoError::fileMissing, file),
              "Die Datei des DrumGizmo-Kits kann nicht gelesen werden: x.xml");
    EXPECT_EQ(KitPresenter::drumGizmoErrorMessage(DrumGizmoError::notADrumkit, file),
              "Die Datei ist kein DrumGizmo-Kit (drumkit.xml): x.xml");
    EXPECT_EQ(KitPresenter::drumGizmoErrorMessage(DrumGizmoError::instrumentMissing, file),
              "Eine Instrument-Datei des DrumGizmo-Kits fehlt: x.xml");
    EXPECT_EQ(KitPresenter::drumGizmoErrorMessage(DrumGizmoError::instrumentBroken, file),
              "Eine Instrument-Datei des DrumGizmo-Kits kann nicht gelesen werden: x.xml");
    EXPECT_EQ(KitPresenter::drumGizmoErrorMessage(DrumGizmoError::outOfMemory, file),
              "Das DrumGizmo-Kit passt nicht in den Arbeitsspeicher: x.xml");
    EXPECT_EQ(KitPresenter::drumGizmoErrorMessage(DrumGizmoError::none, file), "");
}

TEST_F(KitPresenterTest, FDG04_WithoutBackgroundLoaderTheKitIsOnlyNamed)
{
    ON_CALL(drumGizmoReader, read(kDrumkit, false))
        .WillByDefault(Return(engine::DrumGizmoReadResult{.kit = testDrumGizmoKit()}));

    ASSERT_EQ(presenter.loadDrumGizmoKit(kDrumkit), engine::DrumGizmoError::none);

    EXPECT_EQ(presenter.drumGizmoStatus(), "DrumGizmo-Kit „Testkit“");
}

} // namespace
} // namespace drumprog::ui
