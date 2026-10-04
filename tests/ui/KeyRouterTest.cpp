#include "ui/KeyRouter.h"

#include "engine/MockSampleLoader.h"
#include "input/FakeNoteSink.h"
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

namespace scancode = input::scancode;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class KeyRouterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(true));
        ON_CALL(fileSystem, writeText(_, _)).WillByDefault(Return(true));
    }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    juce::UndoManager undoManager;
    NiceMock<engine::MockSampleLoader> loader;
    engine::KitBuilder builder{loader};
    engine::SampleEngine engine;
    engine::KitPublisher kits{builder, engine};
    KitPresenter kit{tree, undoManager, kits, engine};
    NiceMock<io::MockFileSystem> fileSystem;
    io::SettingsStore store{fileSystem, "keymap.txt"};
    input::UsKeyNames keyNames;
    input::Keymap keymap = input::Keymap::defaults();
    KeymapPresenter keymapPresenter{keymap, store, keyNames, kit};
    input::FakeNoteSink sink;
    input::InputActivity activity;
    input::KeyboardInput keyboard{keymap, sink, activity};
    KeyRouter router{keymapPresenter, keyboard};
};

TEST_F(KeyRouterTest, FIN01_KeysTriggerDrumsWhileNotLearning)
{
    EXPECT_TRUE(router.keyDown(scancode::kS, {}, false));
    EXPECT_EQ(sink.hits.size(), 1U);
}

TEST_F(KeyRouterTest, FIN02_TheLearnedKeyDoesNotTrigger)
{
    keymapPresenter.startLearning(3);
    EXPECT_TRUE(router.keyDown(scancode::kA, {}, false));
    EXPECT_TRUE(sink.hits.empty());
    EXPECT_EQ(keymap.keyFor(38), scancode::kA);
}

TEST_F(KeyRouterTest, FIN06_TextFieldFocusIsPassedOn)
{
    EXPECT_FALSE(router.keyDown(scancode::kS, {}, true));
}

TEST_F(KeyRouterTest, FIN01_ReleasedKeysTriggerAgain)
{
    router.keyDown(scancode::kS, {}, false);
    router.releaseKeysNotDown([](int) { return false; });
    router.keyDown(scancode::kS, {}, false);
    EXPECT_EQ(sink.hits.size(), 2U);
}

} // namespace
} // namespace drumprog::ui
