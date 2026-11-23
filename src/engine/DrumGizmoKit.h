#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::engine
{

/// One channel of a DrumGizmo sample: which kit microphone it is and where its audio lies (F-DG-01).
struct DrumGizmoAudioFile
{
    std::string channel; ///< kit channel (microphone), e.g. "OHL", after the instrument's channel map
    std::filesystem::path file;
    int fileChannel = 1; ///< 1-based channel inside the file
};

/// One recorded hit. power is DrumGizmo's measure of how hard it was played (F-DG-01).
struct DrumGizmoSample
{
    std::string name;
    double power = 0.0;
    std::vector<DrumGizmoAudioFile> audioFiles;
};

struct DrumGizmoInstrument
{
    std::string name;
    std::string group; ///< e.g. "hihat": instruments of one group choke each other (F-DG-08)
    std::vector<DrumGizmoSample> samples;
};

/// The content of a DrumGizmo kit: drumkit.xml, the instrument files and, if the kit brings one, its
/// MIDI map. Plain data, read by io::DrumGizmoReader.
struct DrumGizmoKit
{
    std::string name;
    std::vector<std::string> channels;
    std::vector<DrumGizmoInstrument> instruments;
    /// MIDI note -> instrument name from the kit's midimap file; empty without one.
    std::map<int, std::string> midiMap;

    [[nodiscard]] const DrumGizmoInstrument* findInstrument(const std::string& instrumentName) const;
};

enum class DrumGizmoError : std::uint8_t
{
    none,
    fileMissing,       ///< drumkit.xml does not exist or cannot be read
    notADrumkit,       ///< not XML or no <drumkit> element
    instrumentMissing, ///< an instrument file does not exist or cannot be read
    instrumentBroken,  ///< an instrument file is not a DrumGizmo instrument
    outOfMemory,       ///< the samples do not fit into RAM (Q-09)
};

struct DrumGizmoReadResult
{
    std::optional<DrumGizmoKit> kit;
    DrumGizmoError error = DrumGizmoError::none;
    std::filesystem::path errorFile; ///< the file the error is about
};

/// Reads DrumGizmo kits (E-06). Implementations must be usable from a background thread.
class IDrumGizmoReader
{
public:
    virtual ~IDrumGizmoReader() = default;

    /// withSamples false skips the instrument files' sample lists, which is enough to map instruments.
    [[nodiscard]] virtual DrumGizmoReadResult read(const std::filesystem::path& drumkitFile,
                                                   bool withSamples) const = 0;

protected:
    IDrumGizmoReader() = default;
    IDrumGizmoReader(const IDrumGizmoReader&) = default;
    IDrumGizmoReader(IDrumGizmoReader&&) = default;
    IDrumGizmoReader& operator=(const IDrumGizmoReader&) = default;
    IDrumGizmoReader& operator=(IDrumGizmoReader&&) = default;
};

} // namespace drumprog::engine
