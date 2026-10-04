#include "input/EngineNoteSinks.h"

#include "engine/RenderHelpers.h"

#include <gtest/gtest.h>

#include <memory>

namespace drumprog::input
{
namespace
{

double fakeTime() noexcept
{
    return 12.5;
}

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

TEST_F(EngineNoteSinksTest, FIN01_FIN08_KeyboardHitsReachTheEngineWithTheirTime)
{
    GuiNoteSink sink{engine, &fakeTime};
    EXPECT_TRUE(sink.noteOn(36, 100));
    renderBlock();
    EXPECT_EQ(engine.indicators().hitCount(0), 1U);
    ASSERT_EQ(engine.liveHits().size(), 1U);
    EXPECT_DOUBLE_EQ(engine.liveHits()[0].timeSeconds, 12.5);
}

TEST_F(EngineNoteSinksTest, FIN03_FIN08_MidiHitsReachTheEngineWithTheirTime)
{
    MidiNoteSink sink{engine, &fakeTime};
    EXPECT_TRUE(sink.noteOn(36, 100));
    renderBlock();
    EXPECT_EQ(engine.indicators().hitCount(0), 1U);
    ASSERT_EQ(engine.liveHits().size(), 1U);
    EXPECT_DOUBLE_EQ(engine.liveHits()[0].timeSeconds, 12.5);
}

} // namespace
} // namespace drumprog::input
