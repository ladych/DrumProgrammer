#include "engine/TempoMath.h"

#include <gtest/gtest.h>

namespace drumprog::engine
{
namespace
{

TEST(TempoMathTest, FTR06_SamplesPerTickFollowTempoAndRate)
{
    // 120 BPM: one quarter = 0.5 s = 24000 samples at 48 kHz = 960 ticks.
    EXPECT_DOUBLE_EQ(samplesPerTick(120.0, 960, 48000.0), 25.0);
    EXPECT_DOUBLE_EQ(samplesPerTick(60.0, 960, 48000.0), 50.0);
    EXPECT_DOUBLE_EQ(samplesPerTick(120.0, 960, 44100.0), 22.96875);
}

TEST(TempoMathTest, FTR03_BeatIsTheDenominatorNoteValue)
{
    EXPECT_EQ(ticksPerBeat(960, 4), 960);
    EXPECT_EQ(ticksPerBeat(960, 8), 480);
    EXPECT_EQ(ticksPerBeat(960, 16), 240);
}

TEST(TempoMathTest, FTR03_BarLengthFollowsTimeSignature)
{
    EXPECT_EQ(ticksPerBar(960, 4, 4), 3840);
    EXPECT_EQ(ticksPerBar(960, 6, 8), 2880);
    EXPECT_EQ(ticksPerBar(960, 7, 16), 1680);
}

TEST(TempoMathTest, FIN10_WrapFoldsIntoTheLoop)
{
    EXPECT_EQ(wrapTick(0, 7680), 0);
    EXPECT_EQ(wrapTick(7680, 7680), 0);
    EXPECT_EQ(wrapTick(7690, 7680), 10);
    EXPECT_EQ(wrapTick(-10, 7680), 7670);
}

TEST(TempoMathTest, FloorDivRoundsDown)
{
    EXPECT_EQ(floorDiv(7, 2), 3);
    EXPECT_EQ(floorDiv(-7, 2), -4);
    EXPECT_EQ(floorDiv(-8, 2), -4);
}

} // namespace
} // namespace drumprog::engine
