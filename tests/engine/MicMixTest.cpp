#include "engine/MicMix.h"

#include <gtest/gtest.h>

#include <cmath>

namespace drumprog::engine
{
namespace
{

TEST(MicMix, FDG03_OverheadAndAmbienceMicsArePannedBySide)
{
    EXPECT_EQ(micPosition("OHL"), MicPosition::left);
    EXPECT_EQ(micPosition("OHR"), MicPosition::right);
    EXPECT_EQ(micPosition("AmbL"), MicPosition::left);
    EXPECT_EQ(micPosition("Amb-R"), MicPosition::right);
    EXPECT_EQ(micPosition("Overhead Left"), MicPosition::left);
    EXPECT_EQ(micPosition("Room_Right"), MicPosition::right);
    EXPECT_EQ(micPosition("Room"), MicPosition::centre);
}

TEST(MicMix, FDG03_CloseMicsAreCentredEvenWithASideInTheName)
{
    EXPECT_EQ(micPosition("KdrumL"), MicPosition::centre);
    EXPECT_EQ(micPosition("Snare_top"), MicPosition::centre);
    EXPECT_EQ(micPosition("Tom1"), MicPosition::centre);
}

TEST(MicMix, FDG03_TriggerChannelsAreMuted)
{
    EXPECT_EQ(micPosition("Snare_trigger"), MicPosition::muted);
    EXPECT_EQ(micPosition("Trigger"), MicPosition::muted);
    EXPECT_EQ(micPosition("OH_trig"), MicPosition::muted);
}

TEST(MicMix, FDG03_GainsKeepTheLevelOfThePlainDownmix)
{
    const auto centre = micGains(MicPosition::centre, 4);
    EXPECT_FLOAT_EQ(centre.left, 0.5F);
    EXPECT_FLOAT_EQ(centre.right, 0.5F);
    const auto left = micGains(MicPosition::left, 4);
    EXPECT_FLOAT_EQ(left.left, 0.5F * std::sqrt(2.0F));
    EXPECT_FLOAT_EQ(left.right, 0.0F);
    const auto right = micGains(MicPosition::right, 4);
    EXPECT_FLOAT_EQ(right.left, 0.0F);
    EXPECT_FLOAT_EQ(right.right, 0.5F * std::sqrt(2.0F));
    const auto muted = micGains(MicPosition::muted, 4);
    EXPECT_FLOAT_EQ(muted.left + muted.right, 0.0F);
    EXPECT_FLOAT_EQ(micGains(MicPosition::centre, 0).left, 1.0F);
}

} // namespace
} // namespace drumprog::engine
