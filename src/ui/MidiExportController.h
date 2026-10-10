#pragma once

#include "io/IFileSystem.h"
#include "ui/ActivePattern.h"
#include "ui/IMidiExportView.h"

#include <juce_data_structures/juce_data_structures.h>

namespace drumprog::ui
{

/// Menu "MIDI > Export Pattern" (F-MI-01): asks for a file and writes the active pattern there as
/// Standard MIDI File. GUI thread only.
class MidiExportController
{
public:
    MidiExportController(juce::ValueTree project,
                         const ActivePattern& activePattern,
                         io::IFileSystem& fileSystem,
                         IMidiExportView& view);

    [[nodiscard]] bool canExportPattern() const noexcept { return activePattern_.index() >= 0; }
    /// Exports the pattern that is active when the dialog opens, with the tempo at the time of writing.
    void exportActivePattern();

private:
    void write(const juce::ValueTree& pattern, const std::filesystem::path& file);

    juce::ValueTree project_;
    const ActivePattern& activePattern_;
    io::IFileSystem& fileSystem_;
    IMidiExportView& view_;
};

} // namespace drumprog::ui
