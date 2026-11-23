#include "engine/KitPublisher.h"

#include <utility>

namespace drumprog::engine
{
namespace
{

std::filesystem::path drumkitFileOf(const KitDescription& kit)
{
    for (const auto& slot : kit)
        if (!slot.drumkitFile.empty())
            return slot.drumkitFile;
    return {};
}

} // namespace

KitPublisher::KitPublisher(KitBuilder& builder, SampleEngine& engine, DrumGizmoBackgroundLoader* drumGizmo)
    : builder_(builder), engine_(engine), drumGizmoLoader_(drumGizmo)
{
}

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

bool KitPublisher::poll()
{
    if (drumGizmoLoader_ == nullptr)
        return false;
    auto result = drumGizmoLoader_->takeResult();
    if (!result)
        return false;
    drumGizmo_ = std::move(result->kit);
    status_.error = result->error;
    status_.errorFile = std::move(result->errorFile);
    status_.state = drumGizmo_ ? DrumGizmoStatus::State::ready : DrumGizmoStatus::State::failed;
    status_.bytes = drumGizmo_ ? drumGizmo_->bytes : 0;
    status_.missingFiles = drumGizmo_ ? drumGizmo_->missingFiles.size() : 0;
    rebuild();
    return true;
}

void KitPublisher::reloadDrumGizmoKit()
{
    drumkitFile_.clear();
    rebuild();
}

const DrumGizmoStatus& KitPublisher::drumGizmoStatus() const noexcept
{
    return status_;
}

float KitPublisher::drumGizmoProgress() const noexcept
{
    return drumGizmoLoader_ != nullptr ? drumGizmoLoader_->progress() : 0.0F;
}

void KitPublisher::followDrumGizmoKit()
{
    // A kit without slots only appears while a kit is replaced slot by slot; the loaded kit stays.
    if (kit_.empty())
        return;
    const auto file = drumkitFileOf(kit_);
    if (file == drumkitFile_ && (file.empty() || sampleRate_ == drumGizmoRate_))
        return;
    // A new kit or a new device rate: the samples are mixed and resampled again (F-SE-02).
    drumkitFile_ = file;
    drumGizmoRate_ = sampleRate_;
    drumGizmo_.reset();
    status_ = {};
    if (drumGizmoLoader_ == nullptr)
        return;
    if (file.empty())
    {
        drumGizmoLoader_->cancel();
        return;
    }
    status_.state = DrumGizmoStatus::State::loading;
    drumGizmoLoader_->start(file, sampleRate_);
}

void KitPublisher::rebuild()
{
    followDrumGizmoKit();
    auto result = builder_.build(kit_, sampleRate_, drumGizmo_.get());
    missing_ = std::move(result.missingSamples);
    if (drumGizmo_)
        missing_.insert(missing_.end(), drumGizmo_->missingFiles.begin(), drumGizmo_->missingFiles.end());
    engine_.setKit(std::move(result.kit));
}

} // namespace drumprog::engine
