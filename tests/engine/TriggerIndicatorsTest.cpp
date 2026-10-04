#include "engine/TriggerIndicators.h"

#include <gtest/gtest.h>

namespace drumprog::engine
{
namespace
{

TEST(TriggerIndicatorsTest, FSE09_CountsHitsPerSlot)
{
    TriggerIndicators indicators;
    EXPECT_EQ(indicators.hitCount(3), 0U);
    indicators.signal(3);
    indicators.signal(3);
    indicators.signal(4);
    EXPECT_EQ(indicators.hitCount(3), 2U);
    EXPECT_EQ(indicators.hitCount(4), 1U);
}

TEST(TriggerIndicatorsTest, FSE09_IgnoresInvalidSlots)
{
    TriggerIndicators indicators;
    indicators.signal(-1);
    indicators.signal(TriggerIndicators::kMaxSlots);
    EXPECT_EQ(indicators.hitCount(-1), 0U);
    EXPECT_EQ(indicators.hitCount(TriggerIndicators::kMaxSlots), 0U);
    EXPECT_EQ(indicators.hitCount(TriggerIndicators::kMaxSlots - 1), 0U);
}

} // namespace
} // namespace drumprog::engine
