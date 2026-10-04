#include "engine/KitPublisher.h"

#include <utility>

namespace drumprog::engine
{

KitPublisher::KitPublisher(KitBuilder& builder, SampleEngine& engine) : builder_(builder), engine_(engine) {}

void KitPublisher::publishKit(const KitDescription& kit)
{
    kit_ = kit;
    rebuild();
}

void KitPublisher::setDeviceSampleRate(double sampleRate)
{
    sampleRate_ = sampleRate;
    rebuild();
}

bool KitPublisher::canLoad(const std::filesystem::path& file)
{
    return builder_.preload(file);
}

const std::vector<std::filesystem::path>& KitPublisher::missingSamples() const noexcept
{
    return missing_;
}

void KitPublisher::rebuild()
{
    auto result = builder_.build(kit_, sampleRate_);
    missing_ = std::move(result.missingSamples);
    engine_.setKit(std::move(result.kit));
}

} // namespace drumprog::engine
