#pragma once

#include "engine/EngineKit.h"
#include "engine/ISampleLoader.h"
#include "engine/KitDescription.h"

#include <filesystem>
#include <map>
#include <memory>
#include <vector>

namespace drumprog::engine
{

struct KitBuildResult
{
    std::unique_ptr<EngineKit> kit;
    /// Files that could not be loaded; their slots play no sample (F-PJ-03).
    std::vector<std::filesystem::path> missingSamples;
};

/// Builds engine kits from a kit description on the GUI thread. Decoded and
/// resampled samples are cached, so changing gain or pitch does not touch the
/// disk again and a new device sample rate only resamples (F-SE-02).
class KitBuilder
{
public:
    static constexpr int kMaxPitchSemitones = 12;

    explicit KitBuilder(ISampleLoader& loader);

    [[nodiscard]] KitBuildResult build(const KitDescription& kit, double deviceSampleRate);

private:
    [[nodiscard]] std::shared_ptr<const SampleBuffer> sampleFor(const std::filesystem::path& file);

    ISampleLoader& loader_;
    double cachedRate_ = 0.0;
    std::map<std::filesystem::path, std::shared_ptr<const SampleBuffer>> originals_;
    std::map<std::filesystem::path, std::shared_ptr<const SampleBuffer>> resampled_;
};

/// Playback speed for a pitch shift, clamped to +-12 semitones (F-SE-07).
[[nodiscard]] double pitchToPlaybackRate(int semitones) noexcept;

} // namespace drumprog::engine
