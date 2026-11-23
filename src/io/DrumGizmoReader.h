#pragma once

#include "engine/DrumGizmoKit.h"
#include "io/IFileSystem.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace drumprog::io
{

/// An instrument as drumkit.xml lists it, before its own file is read.
struct DrumGizmoInstrumentEntry
{
    std::string name;
    std::string group;
    std::filesystem::path file;
    std::map<std::string, std::string> channelMap; ///< instrument channel -> kit channel
};

/// Reads DrumGizmo kits (F-DG-01): drumkit.xml with the kit's channels and instruments, every instrument
/// file with its samples, their power and which file channel is which microphone, and the kit's MIDI map
/// (a file "*midimap*.xml" next to drumkit.xml) if there is one. Format versions 1.0 and 2.0 are both read:
/// samples without a power attribute take the middle of their velocity range. Only reads files, so it can
/// run on a background thread.
class DrumGizmoReader final : public engine::IDrumGizmoReader
{
public:
    explicit DrumGizmoReader(const IFileSystem& fileSystem);

    [[nodiscard]] engine::DrumGizmoReadResult read(const std::filesystem::path& drumkitFile,
                                                   bool withSamples) const override;

private:
    [[nodiscard]] engine::DrumGizmoReadResult
    readInstruments(engine::DrumGizmoKit kit,
                    const std::vector<DrumGizmoInstrumentEntry>& entries,
                    bool withSamples) const;
    [[nodiscard]] std::map<int, std::string> readMidiMap(const std::filesystem::path& directory) const;

    const IFileSystem& fileSystem_;
};

} // namespace drumprog::io
