#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace drumprog::engine
{

/// Plain description of one kit slot as the engine needs it. The project model
/// (AP1) converts its kit into this form; the engine never reads the model itself.
struct KitSlotDescription
{
    int gmNote = 0;   ///< note of this slot in the GM drum map
    int midiNote = 0; ///< note that triggers the slot, overridable (F-SE-05)
    std::string name;
    std::filesystem::path sampleFile; ///< empty: no sample assigned
    float gain = 1.0F;                ///< linear (F-SE-06)
    int pitchSemitones = 0;           ///< -12..+12 (F-SE-07)
    int chokeGroup = 0;               ///< 0: none (F-SE-08)
    bool coreSlot = true;             ///< shown without expanding the kit panel (F-SE-04)

    /// True while the MIDI note was not overridden, shown as "(GM-Default)".
    [[nodiscard]] bool usesGmNote() const noexcept;
};

using KitDescription = std::vector<KitSlotDescription>;

inline constexpr int kHiHatChokeGroup = 1;

/// Default kit with the GM drum notes 35..59 and no samples (F-SE-04).
/// Closed (42), pedal (44) and open hi-hat (46) share one choke group (F-SE-08).
[[nodiscard]] KitDescription makeGmDefaultKit();

} // namespace drumprog::engine
