#include "ui/KeymapPresenter.h"

#include "engine/MockSampleLoader.h"
#include "input/Scancode.h"
#include "io/MockFileSystem.h"
#include "model/FakeIdGenerator.h"
#include "model/ProjectFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

using input::Keymap;
namespace scancode = input::scancode;
using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

constexpr int kKickRow = 1;     // GM 36, key A
constexpr int kSnareRow = 3;    // GM 38, key S
constexpr int kClosedRow = 7;   // GM 42, key W
constexpr int kOpenRow = 11;    // GM 46, key E
constexpr int kAcousticRow = 0; // GM 35, no key

class KeymapPresenterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(true));
        ON_CALL(fileSystem, writeText(_, _)).WillByDefault(DoAll(SaveArg<1>(&saved), Return(true)));
    }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    juce::ValueTree globalKit = model::ProjectFactory::createDefaultKit();
    juce::UndoManager undoManager;
    NiceMock<engine::MockSampleLoader> loader;
    engine::KitBuilder builder{loader};
    engine::SampleEngine engine;
    engine::KitPublisher kits{builder, engine};
    KitPresenter kit{tree, globalKit, undoManager, kits, engine};

    NiceMock<io::MockFileSystem> fileSystem;
    io::SettingsStore store{fileSystem, "config/keymap.txt"};
    std::string saved;
    input::UsKeyNames keyNames;
    Keymap keymap = Keymap::defaults();
    KeymapPresenter presenter{keymap, store, keyNames, kit};
};

TEST_F(KeymapPresenterTest, FIN02_ListsAllSlotsWithNoteNameAndKey)
{
    EXPECT_EQ(presenter.numRows(), 25);
    EXPECT_EQ(presenter.noteLabel(kSnareRow), "38");
    EXPECT_EQ(presenter.slotName(kSnareRow), "Acoustic Snare");
    EXPECT_EQ(presenter.keyLabel(kSnareRow), "S");
    EXPECT_EQ(presenter.keyLabel(kAcousticRow), "–");
}

TEST_F(KeymapPresenterTest, FIN02_RowsOutsideTheKitAreEmpty)
{
    EXPECT_EQ(presenter.noteLabel(25), "");
    EXPECT_EQ(presenter.slotName(-1), "");
    EXPECT_EQ(presenter.keyLabel(25), "");
    EXPECT_FALSE(presenter.isDuplicate(-1));
}

TEST_F(KeymapPresenterTest, FIN02_KeyFollowsTheSlotsMidiNote)
{
    kit.select(kSnareRow);
    kit.setMidiNote(40);
    EXPECT_EQ(presenter.noteLabel(kSnareRow), "40");
    EXPECT_EQ(presenter.keyLabel(kSnareRow), "–");
}

TEST_F(KeymapPresenterTest, FIN02_LearningAssignsTheNextKeyAndSaves)
{
    presenter.startLearning(kSnareRow);
    EXPECT_EQ(presenter.learningRow(), kSnareRow);
    EXPECT_EQ(presenter.keyLabel(kSnareRow), "Taste drücken …");

    EXPECT_TRUE(presenter.captureKey(scancode::kY));

    EXPECT_FALSE(presenter.learningRow().has_value());
    EXPECT_EQ(keymap.keyFor(38), scancode::kY);
    EXPECT_EQ(Keymap::fromText(saved).keyFor(38), scancode::kY);
}

TEST_F(KeymapPresenterTest, FIN02_KeysAreNotCapturedWithoutLearning)
{
    EXPECT_FALSE(presenter.captureKey(scancode::kY));
    EXPECT_TRUE(saved.empty());
}

TEST_F(KeymapPresenterTest, FIN02_EscapeCancelsLearning)
{
    presenter.startLearning(kSnareRow);
    EXPECT_TRUE(presenter.captureKey(scancode::kEscape));
    EXPECT_FALSE(presenter.learningRow().has_value());
    EXPECT_EQ(keymap.keyFor(38), scancode::kS);
    EXPECT_TRUE(saved.empty());
}

TEST_F(KeymapPresenterTest, FIN02_ModifierKeysKeepLearning)
{
    presenter.startLearning(kSnareRow);
    EXPECT_TRUE(presenter.captureKey(scancode::kLeftShift));
    EXPECT_EQ(presenter.learningRow(), kSnareRow);
}

TEST_F(KeymapPresenterTest, FIN02_CancelLearningAndInvalidRows)
{
    presenter.startLearning(25);
    EXPECT_FALSE(presenter.learningRow().has_value());
    presenter.startLearning(kSnareRow);
    presenter.cancelLearning();
    EXPECT_FALSE(presenter.learningRow().has_value());
}

TEST_F(KeymapPresenterTest, FIN02_DuplicatesAreMarkedAndReported)
{
    EXPECT_TRUE(presenter.duplicateWarning().empty());
    presenter.startLearning(kOpenRow);
    presenter.captureKey(scancode::kW);

    EXPECT_TRUE(presenter.isDuplicate(kOpenRow));
    EXPECT_TRUE(presenter.isDuplicate(kClosedRow));
    EXPECT_FALSE(presenter.isDuplicate(kSnareRow));
    EXPECT_FALSE(presenter.isDuplicate(kAcousticRow));
    EXPECT_EQ(presenter.duplicateWarning(), "Taste W ist doppelt belegt: 42 Closed Hi-Hat, 46 Open Hi-Hat\n");
}

TEST_F(KeymapPresenterTest, FIN02_DuplicateWarningNamesNotesWithoutSlot)
{
    keymap.assign(100, scancode::kA);
    EXPECT_EQ(presenter.duplicateWarning(), "Taste A ist doppelt belegt: 36 Bass Drum 1, 100\n");
}

TEST_F(KeymapPresenterTest, FIN02_ClearRemovesTheKeyAndSaves)
{
    presenter.clearKey(kKickRow);
    presenter.clearKey(25);
    EXPECT_EQ(presenter.keyLabel(kKickRow), "–");
    EXPECT_FALSE(Keymap::fromText(saved).keyFor(36).has_value());
}

TEST_F(KeymapPresenterTest, FIN02_RestoreDefaults)
{
    keymap.assign(38, scancode::kY);
    presenter.startLearning(kKickRow);

    presenter.restoreDefaults();

    EXPECT_EQ(keymap.keyFor(38), scancode::kS);
    EXPECT_FALSE(presenter.learningRow().has_value());
    EXPECT_EQ(Keymap::fromText(saved).keyFor(38), scancode::kS);
}

} // namespace
} // namespace drumprog::ui
