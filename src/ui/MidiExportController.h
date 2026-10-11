#pragma once

#include "io/IFileSystem.h"
#include "io/MidiFileWriter.h"
#include "ui/ActivePattern.h"
#include "ui/IMidiExportView.h"

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <string>

namespace drumprog::ui
{

/// Menu "MIDI > Export" (F-MI-01, F-MI-02, F-MI-05): asks whether to export the active pattern or the song,
/// then for a file, and writes it there as Standard MIDI File. GUI thread only.
class MidiExportController
{
public:
    MidiExportController(juce::ValueTree project,
                         const ActivePattern& activePattern,
                         io::IFileSystem& fileSystem,
                         IMidiExportView& view);

    [[nodiscard]] bool canExportPattern() const noexcept { return activePattern_.index() >= 0; }
    /// The song needs at least one block.
    [[nodiscard]] bool canExportSong() const;
    /// A song block always has its pattern in the project, so there is something to export whenever a
    /// pattern is active.
    [[nodiscard]] bool canExport() const noexcept { return canExportPattern(); }

    /// Export dialog (F-MI-05): the choice between pattern and song, then the file.
    void exportMidi();
    /// Exports the pattern that is active when the file dialog opens, with the tempo at the time of writing.
    void exportActivePattern();
    /// Exports the song as it is when the file is written.
    void exportSong();

private:
    /// `failedMessage` is the English message with %1 for the file.
    void write(const io::MidiPattern& midi, const std::filesystem::path& file, const char* failedMessage);

    juce::ValueTree project_;
    const ActivePattern& activePattern_;
    io::IFileSystem& fileSystem_;
    IMidiExportView& view_;
};

} // namespace drumprog::ui
