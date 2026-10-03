#include "engine/VelocityCurve.h"

#include <cmath>

namespace drumprog::engine
{

float velocityToGain(int velocity) noexcept
{
    constexpr int kMaxVelocity = 127;
    if (velocity <= 0)
        return 0.0F;
    if (velocity >= kMaxVelocity)
        return 1.0F;
    const float db = kVelocityRangeDb * static_cast<float>(velocity - kMaxVelocity) / (kMaxVelocity - 1);
    return std::pow(10.0F, db / 20.0F);
}

} // namespace drumprog::engine
