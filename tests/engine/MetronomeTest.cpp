#include "engine/Metronome.h"

#include "engine/RenderHelpers.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace drumprog::engine
{
namespace
{

constexpr double kRate = 48000.0;
constexpr int kClickLength = 1440; // 30 ms at 48 kHz

class MetronomeTest : public ::testing::Test
{
protected:
    void SetUp() override { metronome.prepare(kRate); }

    StereoOutput render(int frames)
    {
        StereoOutput out(frames);
        metronome.render(out.channels.data(), 2, frames);
        return out;
    }

    Metronome metronome;
};

float peak(const std::vector<float>& samples)
{
    float result = 0.0F;
    for (const float value : samples)
        result = std::max(result, std::abs(value));
    return result;
}

TEST_F(MetronomeTest, FTR08_SilentWithoutClick)
{
    EXPECT_EQ(peak(render(64).left), 0.0F);
}

TEST_F(MetronomeTest, FTR08_ClickStartsAtItsSampleOffsetOnAllChannels)
{
    EXPECT_TRUE(metronome.click(false, 10));
    const auto out = render(64);

    EXPECT_EQ(peak({out.left.begin(), out.left.begin() + 11}), 0.0F); // sine starts at zero
    EXPECT_NE(out.left[11], 0.0F);
    EXPECT_EQ(out.left, out.right);
}

TEST_F(MetronomeTest, FTR08_ClickFadesOutAfter30Ms)
{
    metronome.click(false, 0);
    const auto first = render(kClickLength);
    EXPECT_GT(peak(first.left), 0.3F);
    EXPECT_EQ(peak(render(64).left), 0.0F);
}

TEST_F(MetronomeTest, FTR08_ClickAddsToTheOutput)
{
    metronome.click(false, 0);
    StereoOutput out(4);
    out.left[0] = 1.0F;
    metronome.render(out.channels.data(), 2, 4);
    EXPECT_FLOAT_EQ(out.left[0], 1.0F);
}

TEST_F(MetronomeTest, FTR08_AccentHasHigherPitch)
{
    metronome.click(false, 0);
    const auto normal = render(8);
    metronome.prepare(kRate);
    metronome.click(true, 0);
    const auto accent = render(8);
    EXPECT_GT(accent.left[1], normal.left[1]);
}

TEST_F(MetronomeTest, FTR08_LevelScalesTheClickAndIsClamped)
{
    EXPECT_FLOAT_EQ(metronome.level(), Metronome::kDefaultLevel);
    metronome.setLevel(1.0F);
    metronome.click(false, 0);
    const float full = peak(render(64).left);

    metronome.prepare(kRate);
    metronome.setLevel(0.25F);
    metronome.click(false, 0);
    EXPECT_NEAR(peak(render(64).left), full / 4.0F, 1e-6F);

    metronome.setLevel(2.0F);
    EXPECT_FLOAT_EQ(metronome.level(), 1.0F);
    metronome.setLevel(-1.0F);
    EXPECT_FLOAT_EQ(metronome.level(), 0.0F);
}

TEST_F(MetronomeTest, FTR08_SeveralClicksInOneBlockRestartTheSound)
{
    metronome.click(false, 0);
    metronome.click(true, 100);
    const auto out = render(200);
    EXPECT_EQ(out.left[100], 0.0F); // restarted at phase zero
    EXPECT_NE(out.left[99], 0.0F);
}

TEST_F(MetronomeTest, Q04_RejectsClicksBeyondTheBlockCapacity)
{
    for (int index = 0; index < Metronome::kMaxClicksPerBlock; ++index)
        EXPECT_TRUE(metronome.click(false, index));
    EXPECT_FALSE(metronome.click(false, 50));
    render(64);
    EXPECT_TRUE(metronome.click(false, 0));
}

} // namespace
} // namespace drumprog::engine
