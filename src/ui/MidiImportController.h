#pragma once

#include "io/IFileSystem.h"
#include "io/MidiFileReader.h"
#include "io/MidiImport.h"
#include "model/IIdGenerator.h"
#include "model/Project.h"
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
/// none with a slot adds nothing. The project takes the file's tempo; its time signature stays. If the song
/// is empty, the pattern also becomes its first block, so an imported song plays in the song mode. GUI thread
/// only.
class MidiImportController
{
public:
    static constexpr const char* kUndoName = "MIDI-Import";

    /// What the project takes from the file; empty parts stay as they are.
    struct FileTempo
    {
        double bpm = 0.0;         ///< 0: keep the project's tempo
        int timeSigNumerator = 0; ///< 0: keep the project's time signature
        int timeSigDenominator = 0;
    };

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
    /// One undo step with the tempo taken from the file; the new pattern becomes active and, in an empty
    /// song, its first block.
    void addPattern(const io::MidiFileContent& content, const FileTempo& tempo);
    /// Adds the file kept while the question is open, with or without its tempo.
    void addPending(bool adoptTempo);

    juce::ValueTree project_;
    juce::ValueTree globalKit_;
    juce::UndoManager& undoManager_;
    model::IIdGenerator& idGenerator_;
    ActivePattern& activePattern_;
    io::IFileSystem& fileSystem_;
    IMidiImportView& view_;
    io::MidiFileContent pending_;
    FileTempo pendingTempo_;
};

} // namespace drumprog::ui
