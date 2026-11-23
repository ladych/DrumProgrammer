#include "engine/LayerSelection.h"

#include <gtest/gtest.h>

#include <set>
#include <vector>

namespace drumprog::engine
{
namespace
{

std::vector<LayerSample> layersWithPowers(std::initializer_list<float> powers)
{
    std::vector<LayerSample> layers;
    for (const float power : powers)
        layers.push_back({.sample = nullptr, .power = power});
    return layers;
}

TEST(LayerSelection, FDG02_EmptyInstrumentHasNoLayer)
{
    EXPECT_EQ(selectLayer({}, 100, 0U, -1), -1);
}

TEST(LayerSelection, FDG02_SingleHitIsAlwaysPlayed)
{
    const auto layers = layersWithPowers({1.0F});
    EXPECT_EQ(selectLayer(layers, 1, 7U, 0), 0);
    EXPECT_EQ(selectLayer(layers, 127, 8U, -1), 0);
}

TEST(LayerSelection, FDG02_VelocityPicksTheNearestHitOrANeighbour)
{
    const auto layers = layersWithPowers({0.0F, 0.25F, 0.5F, 0.75F, 1.0F});
    for (std::uint32_t random = 0; random < 6; ++random)
    {
        const int soft = selectLayer(layers, 1, random, -1);
        const int middle = selectLayer(layers, 64, random, -1);
        const int hard = selectLayer(layers, 127, random, -1);
        EXPECT_GE(soft, 0);
        EXPECT_LE(soft, 1);
        EXPECT_GE(middle, 1);
        EXPECT_LE(middle, 3);
        EXPECT_GE(hard, 3);
        EXPECT_LE(hard, 4);
    }
}

TEST(LayerSelection, FDG02_PicksAtRandomAmongTheCandidates)
{
    const auto layers = layersWithPowers({0.0F, 0.25F, 0.5F, 0.75F, 1.0F});
    std::set<int> picked;
    for (std::uint32_t random = 0; random < 3; ++random)
        picked.insert(selectLayer(layers, 64, random, -1));
    EXPECT_EQ(picked, (std::set<int>{1, 2, 3}));
}

TEST(LayerSelection, FDG02_NeverRepeatsTheLastHitIfThereIsAnother)
{
    const auto layers = layersWithPowers({0.0F, 0.5F, 0.5F, 0.5F, 1.0F});
    for (int last = 1; last <= 3; ++last)
        for (std::uint32_t random = 0; random < 8; ++random)
            EXPECT_NE(selectLayer(layers, 64, random, last), last);
}

TEST(LayerSelection, FDG02_OutOfRangeVelocitiesAreClamped)
{
    const auto layers = layersWithPowers({0.0F, 1.0F});
    EXPECT_EQ(selectLayer(layers, 0, 0U, 1), 0);
    EXPECT_EQ(selectLayer(layers, 500, 0U, 0), 1);
}

TEST(LayerSelection, FDG02_VelocityAboveTheHardestHitTakesTheHardest)
{
    const auto layers = layersWithPowers({0.0F, 0.1F, 0.2F});
    EXPECT_EQ(selectLayer(layers, 127, 0U, 1), 2);
}

TEST(LayerSelection, Q04_FastRandomChangesWithEveryCall)
{
    FastRandom random;
    const auto first = random.next();
    const auto second = random.next();
    EXPECT_NE(first, second);
    EXPECT_NE(first, 0U);
}

} // namespace
} // namespace drumprog::engine
