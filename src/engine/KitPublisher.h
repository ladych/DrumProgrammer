#pragma once

#include "engine/DrumGizmoLoader.h"
#include "engine/IKitSink.h"
#include "engine/KitBuilder.h"
#include "engine/SampleEngine.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace drumprog::engine
{

/// How far the DrumGizmo kit of the published kit is (F-DG-09, F-DG-10).
struct DrumGizmoStatus
{
    enum class State : std::uint8_t
    {
        none, ///< the kit is a sample kit
        loading,
        ready,
        failed,
    };

    State state = State::none;
    DrumGizmoError error = DrumGizmoError::none;
    std::filesystem::path errorFile;
    std::uint64_t bytes = 0;      ///< RAM of the loaded samples
    std::size_t missingFiles = 0; ///< audio files that could not be loaded
};

/// Turns kit descriptions into engine kits and hands them to the SampleEngine. Rebuilds the last
/// kit when the device sample rate changes, because samples are resampled when the kit is built
/// (F-SE-02). A DrumGizmo kit loads on the background loader; until it is ready its slots play nothing,
/// then poll() publishes it. GUI thread only.
class KitPublisher final : public IKitSink
{
public:
    static constexpr double kDefaultSampleRate = 48000.0;

    /// Without a DrumGizmo loader, DrumGizmo slots stay silent.
    KitPublisher(KitBuilder& builder, SampleEngine& engine, DrumGizmoBackgroundLoader* drumGizmo = nullptr);

    void publishKit(const KitDescription& kit) override;
    void setDeviceSampleRate(double sampleRate);

    /// Loads a file before it is assigned to a slot; false if it cannot be loaded.
    [[nodiscard]] bool canLoad(const std::filesystem::path& file);
    /// Files of the last published kit that could not be loaded (F-PJ-03).
    [[nodiscard]] const std::vector<std::filesystem::path>& missingSamples() const noexcept;

    /// Call from the GUI timer: publishes a DrumGizmo kit that finished loading. True if the status changed.
    bool poll();
    /// Loads the DrumGizmo kit again, e.g. after its files were fixed.
    void reloadDrumGizmoKit();
    [[nodiscard]] const DrumGizmoStatus& drumGizmoStatus() const noexcept;
    /// 0..1 while the DrumGizmo kit loads.
    [[nodiscard]] float drumGizmoProgress() const noexcept;

private:
    void rebuild();
    void followDrumGizmoKit();

    KitBuilder& builder_;
    SampleEngine& engine_;
    DrumGizmoBackgroundLoader* drumGizmoLoader_;
    KitDescription kit_;
    double sampleRate_ = kDefaultSampleRate;
    std::vector<std::filesystem::path> missing_;
    std::filesystem::path drumkitFile_; ///< DrumGizmo kit that is loaded or loading
    double drumGizmoRate_ = 0.0;
    std::shared_ptr<const LoadedDrumGizmoKit> drumGizmo_;
    DrumGizmoStatus status_;
};

} // namespace drumprog::engine
