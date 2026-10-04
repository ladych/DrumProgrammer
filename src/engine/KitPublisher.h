#pragma once

#include "engine/IKitSink.h"
#include "engine/KitBuilder.h"
#include "engine/SampleEngine.h"

#include <filesystem>
#include <vector>

namespace drumprog::engine
{

/// Turns kit descriptions into engine kits and hands them to the SampleEngine. Rebuilds the last
/// kit when the device sample rate changes, because samples are resampled when the kit is built
/// (F-SE-02). GUI thread only.
class KitPublisher final : public IKitSink
{
public:
    static constexpr double kDefaultSampleRate = 48000.0;

    KitPublisher(KitBuilder& builder, SampleEngine& engine);

    void publishKit(const KitDescription& kit) override;
    void setDeviceSampleRate(double sampleRate);

    /// Loads a file before it is assigned to a slot; false if it cannot be loaded.
    [[nodiscard]] bool canLoad(const std::filesystem::path& file);
    /// Files of the last published kit that could not be loaded (F-PJ-03).
    [[nodiscard]] const std::vector<std::filesystem::path>& missingSamples() const noexcept;

private:
    void rebuild();

    KitBuilder& builder_;
    SampleEngine& engine_;
    KitDescription kit_;
    double sampleRate_ = kDefaultSampleRate;
    std::vector<std::filesystem::path> missing_;
};

} // namespace drumprog::engine
