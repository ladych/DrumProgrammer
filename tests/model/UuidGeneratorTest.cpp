#include "model/UuidGenerator.h"

#include <gtest/gtest.h>

namespace drumprog::model
{
namespace
{

TEST(UuidGeneratorTest, FSO01_CreatesDashedUuids)
{
    UuidGenerator generator;

    const auto id = generator.next();

    EXPECT_EQ(id.size(), 36U);
    EXPECT_EQ(id[8], '-');
    EXPECT_EQ(id[23], '-');
}

TEST(UuidGeneratorTest, FSO01_CreatesDifferentIdsEachTime)
{
    UuidGenerator generator;

    EXPECT_NE(generator.next(), generator.next());
}

} // namespace
} // namespace drumprog::model
