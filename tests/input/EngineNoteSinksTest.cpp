#include "input/EngineNoteSinks.h"

#include "engine/RenderHelpers.h"

#include <gtest/gtest.h>

#include <memory>

namespace drumprog::input
{
namespace
{

class EngineNoteSinksTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        engine.prepare(48000.0);
        engine.setKit(std::make_unique<engine::EngineKit>(
            std::vector<engine::EngineSlot>{engine::EngineSlot{.slotIndex = 0, .midiNote = 36}}));
    }

    void renderBlock()
    {
        engine::StereoOutput out(1);
        engine.render(out.channels.data(), 2, 1);
    }

    engine::SampleEngine engine;
};

TEST_F(EngineNoteSinksTest, FIN01_KeyboardHitsReachTheEngine)
{
    GuiNoteSink sink{engine};
    EXPECT_TRUE(sink.noteOn(36, 100));
    renderBlock();
    EXPECT_EQ(engine.indicators().hitCount(0), 1U);
}

TEST_F(EngineNoteSinksTest, FIN03_MidiHitsReachTheEngine)
{
    MidiNoteSink sink{engine};
    EXPECT_TRUE(sink.noteOn(36, 100));
    renderBlock();
    EXPECT_EQ(engine.indicators().hitCount(0), 1U);
}

} // namespace
} // namespace drumprog::input
