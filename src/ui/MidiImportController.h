#pragma once

#include "io/IFileSystem.h"
#include "io/MidiFileReader.h"
#include "io/MidiImport.h"
#include "model/IIdGenerator.h"
#include "ui/ActivePattern.h"
#include "ui/IMidiImportView.h"

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <optional>
#include <string>

namespace drumprog::ui
{

/// Menu "MIDI > Import" (F-MI-03, F-MI-04): asks for a .mid file and adds its notes as a new pattern, one
/// undo step, which becomes the active pattern. The pattern is named after the file's track name, otherwise
/// after the file. Notes without a slot in the kit are reported; a file that cannot be read, has no notes or
/// none with a slot adds nothing. The project's tempo and time signature stay as they are. GUI thread only.
class MidiImportController
{
public:
    static constexpr const char* kUndoName = "MIDI-Import";

    MidiImportController(juce::ValueTree project,
                         juce::ValueTree globalKit,
                         juce::UndoManager& undoManager,
                         model::IIdGenerator& idGenerator,
                         ActivePattern& activePattern,
                         io::IFileSystem& fileSystem,
                         IMidiImportView& view);

    void importMidiFile();

private:
    void import(const std::filesystem::path& file);
    /// std::nullopt if the file cannot be read or is no MIDI file.
    [[nodiscard]] std::optional<io::MidiFileContent> read(const std::filesystem::path& file) const;
    /// One undo step; the new pattern becomes active.
    void addPattern(const std::string& name, const io::ImportedPattern& imported);

    juce::ValueTree project_;
    juce::ValueTree globalKit_;
    juce::UndoManager& undoManager_;
    model::IIdGenerator& idGenerator_;
    ActivePattern& activePattern_;
    io::IFileSystem& fileSystem_;
    IMidiImportView& view_;
};

} // namespace drumprog::ui
