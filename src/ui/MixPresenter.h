#pragma once

#include "model/Project.h"
#include "model/TreeChangeListener.h"

#include <juce_data_structures/juce_data_structures.h>

#include <array>
#include <cstdint>
#include <string>

namespace drumprog::ui
{

enum class MixChannel : std::uint8_t
{
    backing,
    drums, ///< sequenced and live drums
    master
};

/// Logic of the faders "Mix (Referenz)" in the inspector (F-BT-06; humble view app/MixPanel). The gains
/// are part of the project; repeated changes of one fader form one undo step. GUI thread only.
class MixPresenter
{
public:
    static constexpr double kMinGainDb = -60.0; ///< and below: silent
    static constexpr double kMaxGainDb = 6.0;
    static constexpr std::array kChannels{MixChannel::backing, MixChannel::drums, MixChannel::master};

    MixPresenter(juce::ValueTree project, juce::UndoManager& undoManager);

    [[nodiscard]] static std::string label(MixChannel channel);
    /// kMinGainDb for silence.
    [[nodiscard]] double gainDb(MixChannel channel) const;
    /// Clamped to kMinGainDb..kMaxGainDb; kMinGainDb is silence.
    void setGainDb(MixChannel channel, double gainDb);

    /// Changes whenever the project changed, also by undo or loading.
    [[nodiscard]] std::uint32_t changeCount() const noexcept { return changeCount_; }

private:
    [[nodiscard]] model::Mix mix() const;

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
    std::uint32_t changeCount_ = 0;
    model::TreeChangeListener listener_;
};

} // namespace drumprog::ui
