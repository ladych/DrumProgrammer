#include "engine/DrumGizmoLoader.h"

#include "engine/MicMix.h"
#include "engine/Resampler.h"

#include <algorithm>
#include <new>
#include <utility>

namespace drumprog::engine
{
namespace
{

using LoadedFiles = std::map<std::filesystem::path, std::optional<SampleBuffer>>;

const SampleBuffer* fileFor(const std::filesystem::path& file,
                            ISampleLoader& loader,
                            LoadedFiles& files,
                            std::vector<std::filesystem::path>& missing)
{
    auto [entry, inserted] = files.try_emplace(file);
    if (inserted)
    {
        entry->second = loader.load(file);
        if (!entry->second && std::ranges::find(missing, file) == missing.end())
            missing.push_back(file);
    }
    const auto& loaded = entry->second;
    if (!loaded)
        return nullptr;
    return &*loaded;
}

void addChannel(SampleBuffer& mix, const std::vector<float>& channel, MicGains gains)
{
    auto& left = mix.channels[0];
    auto& right = mix.channels[1];
    if (channel.size() > left.size())
    {
        left.resize(channel.size(), 0.0F);
        right.resize(channel.size(), 0.0F);
    }
    for (std::size_t i = 0; i < channel.size(); ++i)
    {
        left[i] += gains.left * channel[i];
        right[i] += gains.right * channel[i];
    }
}

/// Mixes the microphones of one hit to stereo (F-DG-03); null if none of its files could be loaded.
std::optional<SampleBuffer>
mixHit(const DrumGizmoSample& hit, ISampleLoader& loader, std::vector<std::filesystem::path>& missing)
{
    const auto activeMics = std::ranges::count_if(
        hit.audioFiles, [](const auto& file) { return micPosition(file.channel) != MicPosition::muted; });
    LoadedFiles files;
    SampleBuffer mix;
    mix.channels.resize(2);
    bool anyLoaded = false;
    for (const auto& audioFile : hit.audioFiles)
    {
        const auto position = micPosition(audioFile.channel);
        if (position == MicPosition::muted)
            continue;
        const auto* file = fileFor(audioFile.file, loader, files, missing);
        const auto channel = static_cast<std::size_t>(audioFile.fileChannel - 1);
        if (file == nullptr || channel >= file->channels.size())
            continue;
        mix.sampleRate = file->sampleRate;
        anyLoaded = true;
        addChannel(mix, file->channels[channel], micGains(position, static_cast<int>(activeMics)));
    }
    if (!anyLoaded)
        return std::nullopt;
    return mix;
}

std::uint64_t bytesOf(const SampleBuffer& sample)
{
    std::uint64_t bytes = 0;
    for (const auto& channel : sample.channels)
        bytes += channel.size() * sizeof(float);
    return bytes;
}

/// Power relative to the softest and hardest hit of the instrument, so velocity 1..127 spans every hit.
void normalisePowers(std::vector<LayerSample>& layers, double minPower, double maxPower)
{
    const double span = maxPower - minPower;
    for (auto& layer : layers)
        layer.power = span > 0.0 ? static_cast<float>((layer.power - minPower) / span) : 1.0F;
    std::ranges::stable_sort(layers, {}, &LayerSample::power);
}

int countHits(const DrumGizmoKit& kit)
{
    int hits = 0;
    for (const auto& instrument : kit.instruments)
        hits += static_cast<int>(instrument.samples.size());
    return hits;
}

} // namespace

LoadedDrumGizmoKit loadDrumGizmoSamples(const DrumGizmoKit& kit,
                                        ISampleLoader& loader,
                                        double deviceSampleRate,
                                        DrumGizmoLoadProgress& progress)
{
    progress.total = countHits(kit);
    LoadedDrumGizmoKit loaded;
    for (const auto& instrument : kit.instruments)
    {
        std::vector<LayerSample> layers;
        double minPower = 0.0;
        double maxPower = 0.0;
        for (const auto& hit : instrument.samples)
        {
            if (progress.cancelled)
                return loaded;
            if (auto mix = mixHit(hit, loader, loaded.missingFiles))
            {
                auto sample = std::make_shared<const SampleBuffer>(resample(*mix, deviceSampleRate));
                loaded.bytes += bytesOf(*sample);
                minPower = layers.empty() ? hit.power : std::min(minPower, hit.power);
                maxPower = layers.empty() ? hit.power : std::max(maxPower, hit.power);
                layers.push_back({.sample = std::move(sample), .power = static_cast<float>(hit.power)});
            }
            ++progress.done;
        }
        normalisePowers(layers, minPower, maxPower);
        loaded.instruments[instrument.name] = std::move(layers);
    }
    return loaded;
}

DrumGizmoBackgroundLoader::DrumGizmoBackgroundLoader(const IDrumGizmoReader& reader, ISampleLoader& loader)
    : reader_(reader), loader_(loader)
{
}

DrumGizmoBackgroundLoader::~DrumGizmoBackgroundLoader()
{
    cancel();
}

void DrumGizmoBackgroundLoader::start(const std::filesystem::path& drumkitFile, double deviceSampleRate)
{
    cancel();
    progress_.cancelled = false;
    progress_.done = 0;
    progress_.total = 0;
    loading_ = true;
    // Only the loading thread reads the target until it is joined again.
    drumkitFile_ = drumkitFile;
    sampleRate_ = deviceSampleRate;
    thread_ = std::thread{&DrumGizmoBackgroundLoader::run, this};
}

void DrumGizmoBackgroundLoader::cancel()
{
    progress_.cancelled = true;
    waitUntilFinished();
    const std::scoped_lock lock{mutex_};
    result_.reset();
}

std::optional<DrumGizmoBackgroundLoader::Result> DrumGizmoBackgroundLoader::takeResult()
{
    const std::scoped_lock lock{mutex_};
    return std::exchange(result_, std::nullopt);
}

bool DrumGizmoBackgroundLoader::isLoading() const noexcept
{
    return loading_;
}

float DrumGizmoBackgroundLoader::progress() const noexcept
{
    const int total = progress_.total;
    return total > 0 ? static_cast<float>(progress_.done) / static_cast<float>(total) : 0.0F;
}

void DrumGizmoBackgroundLoader::waitUntilFinished()
{
    if (thread_.joinable())
        thread_.join();
}

void DrumGizmoBackgroundLoader::run()
{
    auto result = load(drumkitFile_, sampleRate_);
    {
        // A cancelled load's result is dropped by cancel() once this thread has finished.
        const std::scoped_lock lock{mutex_};
        result_ = std::move(result);
    }
    loading_ = false;
}

DrumGizmoBackgroundLoader::Result DrumGizmoBackgroundLoader::load(const std::filesystem::path& drumkitFile,
                                                                  double deviceSampleRate)
{
    auto read = reader_.read(drumkitFile, true);
    if (!read.kit)
        return {.error = read.error, .errorFile = std::move(read.errorFile), .kit = nullptr};
    try
    {
        auto kit = loadDrumGizmoSamples(*read.kit, loader_, deviceSampleRate, progress_);
        return {.error = DrumGizmoError::none,
                .errorFile = {},
                .kit = std::make_shared<const LoadedDrumGizmoKit>(std::move(kit))};
    }
    catch (...)
    {
        // std::bad_alloc, or a container that cannot grow any more: the kit does not fit into RAM.
        return {.error = DrumGizmoError::outOfMemory, .errorFile = drumkitFile, .kit = nullptr};
    }
}

} // namespace drumprog::engine
