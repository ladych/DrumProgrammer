#include "ui/MidiImportController.h"

#include "i18n/Translate.h"
#include "io/MidiFileReader.h"
#include "io/MidiImport.h"
#include "io/Utf8Path.h"
#include "model/Project.h"
#include "ui/TempoPresenter.h"

#include <functional>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace drumprog::ui
{
namespace
{

constexpr const char* kFailedTitle = i18n::marked("Import failed");

/// "MIDI note 60" or "MIDI notes 60, 61".
std::string midiNotesText(const std::vector<int>& notes)
{
    std::string list;
    for (const int note : notes)
        list += (list.empty() ? "" : ", ") + std::to_string(note);
    return notes.size() == 1 ? i18n::tr("MIDI note %1", list) : i18n::tr("MIDI notes %1", list);
}

std::string unmatchedText(const io::ImportedPattern& imported)
{
    const auto count = std::to_string(imported.numUnmatched);
    const auto notes = midiNotesText(imported.unmatchedNotes);
    if (imported.numUnmatched == 1)
        return i18n::tr("%1 note without a matching slot in the kit was not imported (%2).", count, notes);
    return i18n::tr("%1 notes without a matching slot in the kit were not imported (%2).", count, notes);
}

/// Tempo and time signature of the file that differ from the project and are valid there.
MidiImportController::FileTempo tempoOf(const io::MidiFileContent& file, const model::Project& project)
{
    MidiImportController::FileTempo tempo;
    const double bpm = TempoPresenter::validBpm(file.bpm);
    if (file.bpm > 0.0 && bpm != project.bpm())
        tempo.bpm = bpm;
    const auto signature = project.timeSignature();
    const bool valid = file.timeSigNumerator >= 1 && file.timeSigNumerator <= TempoPresenter::kMaxNumerator &&
                       TempoPresenter::isValidDenominator(file.timeSigDenominator);
    if (valid &&
        (file.timeSigNumerator != signature.numerator || file.timeSigDenominator != signature.denominator))
    {
        tempo.timeSigNumerator = file.timeSigNumerator;
        tempo.timeSigDenominator = file.timeSigDenominator;
    }
    return tempo;
}

std::string timeSignatureText(model::TimeSignature signature)
{
    return std::to_string(signature.numerator) + "/" + std::to_string(signature.denominator);
}

std::string bpmText(double bpm)
{
    std::ostringstream text;
    text << std::fixed << std::setprecision(2) << bpm << " BPM";
    return text.str();
}

std::string adoptQuestion(const MidiImportController::FileTempo& tempo, const model::Project& project)
{
    std::string question;
    if (tempo.bpm > 0.0)
        question +=
            i18n::tr("Tempo of the file: %1 (project: %2)", bpmText(tempo.bpm), bpmText(project.bpm())) +
            "\n";
    if (tempo.timeSigNumerator > 0)
        question += i18n::tr("Time signature of the file: %1 (project: %2)",
                             timeSignatureText({tempo.timeSigNumerator, tempo.timeSigDenominator}),
                             timeSignatureText(project.timeSignature())) +
                    "\n";
    return question + "\n" + i18n::tr("Adopt into the project?");
}

/// Why a file added no pattern.
std::string nothingImportedText(const io::ImportedPattern& imported, const std::string& fileName)
{
    if (imported.numUnmatched == 0)
        return i18n::tr("\"%1\" contains no notes.", fileName);
    return i18n::tr("No note of \"%1\" matches a slot in the kit (%2).",
                    fileName,
                    midiNotesText(imported.unmatchedNotes));
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
    const auto fileName = io::utf8FromPath(file.filename());
    auto content = read(file);
    if (!content)
    {
        view_.showMessage(i18n::tr(kFailedTitle), i18n::tr("\"%1\" is not a readable MIDI file.", fileName));
        return;
    }

    const model::Project project{project_, nullptr};
    const auto imported = io::importedPatternOf(*content, project, project.activeKit(globalKit_));
    if (imported.notes.empty())
    {
        view_.showMessage(i18n::tr(kFailedTitle), nothingImportedText(imported, fileName));
        return;
    }

    if (content->name.empty())
        content->name = io::utf8FromPath(file.stem());
    const auto tempo = tempoOf(*content, project);
    if (tempo.bpm == 0.0 && tempo.timeSigNumerator == 0)
    {
        addPattern(*content, {});
        return;
    }
    pending_ = std::move(*content);
    pendingTempo_ = tempo;
    view_.askToAdoptTempo(adoptQuestion(tempo, project), [this](bool adopt) { addPending(adopt); });
}

std::optional<io::MidiFileContent> MidiImportController::read(const std::filesystem::path& file) const
{
    const auto bytes = fileSystem_.readText(file);
    if (!bytes)
        return std::nullopt;
    return io::readMidiFile(*bytes);
}

void MidiImportController::addPending(bool adoptTempo)
{
    FileTempo tempo;
    if (adoptTempo)
        tempo = pendingTempo_;
    addPattern(pending_, tempo);
    pending_ = {};
}

void MidiImportController::addPattern(const io::MidiFileContent& content, const FileTempo& tempo)
{
    model::Project project{project_, &undoManager_};
    undoManager_.beginNewTransaction(i18n::trJuce(kUndoName));
    if (tempo.bpm > 0.0)
        project.setBpm(tempo.bpm);
    if (tempo.timeSigNumerator > 0)
        project.setTimeSignature({tempo.timeSigNumerator, tempo.timeSigDenominator});
    // The bars follow the time signature the project has now.
    const auto imported = io::importedPatternOf(content, project, project.activeKit(globalKit_));
    const auto id = idGenerator_.next();
    project.addPattern(id, content.name, imported.lengthBars, imported.notes);
    auto song = project.song();
    if (song.numEntries() == 0)
        song.addEntry(id, 0);
    undoManager_.beginNewTransaction();
    activePattern_.select(project.numPatterns() - 1);
    if (imported.numUnmatched > 0)
        view_.showMessage(i18n::tr("MIDI import"), unmatchedText(imported));
}

} // namespace drumprog::ui
