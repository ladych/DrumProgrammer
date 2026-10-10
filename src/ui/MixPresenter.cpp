#include "ui/MixPresenter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace drumprog::ui
{
namespace
{

constexpr double kDecibelFactor = 20.0;

} // namespace

MixPresenter::MixPresenter(juce::ValueTree project, juce::UndoManager& undoManager)
    : project_(std::move(project)), undoManager_(undoManager), listener_(project_, [this] { ++changeCount_; })
{
}

std::string MixPresenter::label(MixChannel channel)
{
    static constexpr std::array kLabels{"Backing", "Drums", "Master"};
    return kLabels.at(static_cast<std::size_t>(channel));
}

double MixPresenter::gainDb(MixChannel channel) const
{
    const auto mix = this->mix();
    double gain = mix.masterGain();
    if (channel == MixChannel::backing)
        gain = mix.backingGain();
    else if (channel == MixChannel::drums)
        gain = mix.drumsGain();
    if (gain <= 0.0)
        return kMinGainDb;
    return std::max(kDecibelFactor * std::log10(gain), kMinGainDb);
}

void MixPresenter::setGainDb(MixChannel channel, double gainDb)
{
    const double clamped = std::clamp(gainDb, kMinGainDb, kMaxGainDb);
    const double gain = clamped <= kMinGainDb ? 0.0 : std::pow(10.0, clamped / kDecibelFactor);
    const auto name = juce::String::fromUTF8(("Mix " + label(channel)).c_str());
    if (undoManager_.getCurrentTransactionName() != name)
        undoManager_.beginNewTransaction(name);
    auto mix = this->mix();
    if (channel == MixChannel::backing)
        mix.setBackingGain(gain);
    else if (channel == MixChannel::drums)
        mix.setDrumsGain(gain);
    else
        mix.setMasterGain(gain);
}

model::Mix MixPresenter::mix() const
{
    return model::Project{project_, &undoManager_}.mix();
}

} // namespace drumprog::ui
