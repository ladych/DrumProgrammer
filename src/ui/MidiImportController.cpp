#include "ui/MidiImportController.h"

#include "io/MidiFileReader.h"
#include "io/MidiImport.h"
#include "io/Utf8Path.h"
#include "model/Project.h"
#include "ui/TempoPresenter.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace drumprog::ui
{
namespace
{

constexpr const char* kFailedTitle = "Import fehlgeschlagen";

/// "MIDI-Note 60" or "MIDI-Noten 60, 61".
std::string midiNotesText(const std::vector<int>& notes)
{
    std::string text = notes.size() == 1 ? "MIDI-Note" : "MIDI-Noten";
    const char* separator = " ";
    for (const int note : notes)
    {
        text += separator + std::to_string(note);
        separator = ", ";
    }
    return text;
}

std::string unmatchedText(const io::ImportedPattern& imported)
{
    const char* notes = " Noten ohne passenden Slot im Kit wurden";
    if (imported.numUnmatched == 1)
        notes = " Note ohne passenden Slot im Kit wurde";
    return std::to_string(imported.numUnmatched) + notes + " nicht übernommen (" +
           midiNotesText(imported.unmatchedNotes) + ").";
}

/// Why a file added no pattern.
std::string nothingImportedText(const io::ImportedPattern& imported, const std::string& fileName)
{
    if (imported.numUnmatched == 0)
        return fileName + " enthält keine Noten.";
    return "Keine Note aus " + fileName + " passt zu einem Slot im Kit (" +
           midiNotesText(imported.unmatchedNotes) + ").";
}

} // namespace

MidiImportController::MidiImportController(juce::ValueTree project,
                                           juce::ValueTree globalKit,
                                           juce::UndoManager& undoManager,
                                           model::IIdGenerator& idGenerator,
                                           ActivePattern& activePattern,
                                           io::IFileSystem& fileSystem,
                                           IMidiImportView& view)
    : project_(std::move(project)), globalKit_(std::move(globalKit)), undoManager_(undoManager),
      idGenerator_(idGenerator), activePattern_(activePattern), fileSystem_(fileSystem), view_(view)
{
}

void MidiImportController::importMidiFile()
{
    view_.chooseMidiFileToOpen(
        [this](std::optional<std::filesystem::path> file)
        {
            if (file)
                import(*file);
        });
}

void MidiImportController::import(const std::filesystem::path& file)
{
    const auto fileName = "„" + io::utf8FromPath(file.filename()) + "“";
    const auto content = read(file);
    if (!content)
    {
        view_.showMessage(kFailedTitle, fileName + " ist keine lesbare MIDI-Datei.");
        return;
    }

    const model::Project project{project_, nullptr};
    const auto imported = io::importedPatternOf(*content, project, project.activeKit(globalKit_));
    if (imported.notes.empty())
    {
        view_.showMessage(kFailedTitle, nothingImportedText(imported, fileName));
        return;
    }

    std::string name = content->name;
    if (name.empty())
        name = io::utf8FromPath(file.stem());
    addPattern(name, imported, content->bpm);
    if (imported.numUnmatched > 0)
        view_.showMessage("MIDI-Import", unmatchedText(imported));
}

std::optional<io::MidiFileContent> MidiImportController::read(const std::filesystem::path& file) const
{
    const auto bytes = fileSystem_.readText(file);
    if (!bytes)
        return std::nullopt;
    return io::readMidiFile(*bytes);
}

void MidiImportController::addPattern(const std::string& name,
                                      const io::ImportedPattern& imported,
                                      double bpm)
{
    model::Project project{project_, &undoManager_};
    undoManager_.beginNewTransaction(kUndoName);
    if (bpm > 0.0)
        project.setBpm(TempoPresenter::validBpm(bpm));
    const auto id = idGenerator_.next();
    auto pattern = project.addPattern(id, name, imported.lengthBars);
    for (const auto& note : imported.notes)
        pattern.addNote(note);
    auto song = project.song();
    if (song.numEntries() == 0)
        song.addEntry(id, 0);
    undoManager_.beginNewTransaction();
    activePattern_.select(project.numPatterns() - 1);
}

} // namespace drumprog::ui
