#pragma once

#include "engine/DrumGizmoKit.h"
#include "engine/EngineKit.h"
#include "engine/ISampleLoader.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace drumprog::engine
{

/// The samples of a DrumGizmo kit as the engine plays them: every hit already mixed to stereo (F-DG-03) and
/// converted to the device rate, so the callback never mixes microphones (Q-05).
struct LoadedDrumGizmoKit
{
    /// Instrument name -> its hits, sorted by power (F-DG-02).
    std::map<std::string, std::vector<LayerSample>> instruments;
    /// Audio files that are missing or cannot be decoded; their channels stay silent (Q-09).
    std::vector<std::filesystem::path> missingFiles;
    std::uint64_t bytes = 0; ///< RAM of all samples (F-DG-10)
};

/// Shared between the loading thread and the GUI thread.
struct DrumGizmoLoadProgress
{
    std::atomic<int> done{0};
    std::atomic<int> total{0};
    std::atomic<bool> cancelled{false};
};

/// Loads, mixes and resamples every hit of the kit. Stops early, with what it has, once cancelled is set.
/// Throws std::bad_alloc if the kit does not fit into RAM.
[[nodiscard]] LoadedDrumGizmoKit loadDrumGizmoSamples(const DrumGizmoKit& kit,
                                                      ISampleLoader& loader,
                                                      double deviceSampleRate,
                                                      DrumGizmoLoadProgress& progress);

/// Loads a DrumGizmo kit on a background thread, so a large kit neither blocks the start (Q-08) nor the
/// GUI. The GUI thread starts a load and collects the result; only one load runs at a time.
class DrumGizmoBackgroundLoader
{
public:
    struct Result
    {
        DrumGizmoError error = DrumGizmoError::none;
        std::filesystem::path errorFile;
        std::shared_ptr<const LoadedDrumGizmoKit> kit; ///< null on error
    };

    /// reader and loader are used on the loading thread only.
    DrumGizmoBackgroundLoader(const IDrumGizmoReader& reader, ISampleLoader& loader);
    ~DrumGizmoBackgroundLoader();

    DrumGizmoBackgroundLoader(const DrumGizmoBackgroundLoader&) = delete;
    DrumGizmoBackgroundLoader& operator=(const DrumGizmoBackgroundLoader&) = delete;
    DrumGizmoBackgroundLoader(DrumGizmoBackgroundLoader&&) = delete;
    DrumGizmoBackgroundLoader& operator=(DrumGizmoBackgroundLoader&&) = delete;

    /// Cancels a running load and starts loading this kit.
    void start(const std::filesystem::path& drumkitFile, double deviceSampleRate);
    /// Stops a running load and drops its result; waits for the loading thread.
    void cancel();
    /// The result once the load has finished, only once.
    [[nodiscard]] std::optional<Result> takeResult();
    [[nodiscard]] bool isLoading() const noexcept;
    /// 0..1 while loading.
    [[nodiscard]] float progress() const noexcept;
    /// Blocks until the running load has finished, e.g. in tests.
    void waitUntilFinished();

private:
    void run();
    [[nodiscard]] Result load(const std::filesystem::path& drumkitFile, double deviceSampleRate);

    const IDrumGizmoReader& reader_;
    ISampleLoader& loader_;
    DrumGizmoLoadProgress progress_;
    std::atomic<bool> loading_{false};
    std::mutex mutex_;
    std::optional<Result> result_;      ///< guarded by mutex_
    std::filesystem::path drumkitFile_; ///< target of the running load
    double sampleRate_ = 0.0;
    std::thread thread_;
};

} // namespace drumprog::engine
