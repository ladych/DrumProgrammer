#include "ui/MidiExportController.h"

#include "io/PatternMidiExport.h"
#include "io/SongMidiExport.h"
#include "io/Utf8Path.h"
#include "model/Project.h"
#include "model/SongLayout.h"

#include <optional>
#include <utility>

namespace drumprog::ui
{

MidiExportController::MidiExportController(juce::ValueTree project,
                                           const ActivePattern& activePattern,
                                           io::IFileSystem& fileSystem,
                                           IMidiExportView& view)
    : project_(std::move(project)), activePattern_(activePattern), fileSystem_(fileSystem), view_(view)
{
}

bool MidiExportController::canExportSong() const
{
    return !model::layoutSong(model::Project{project_, nullptr}).empty();
}

void MidiExportController::exportMidi()
{
    if (!canExport())
        return;
    view_.chooseExportKind(canExportPattern(),
                           canExportSong(),
                           [this](std::optional<MidiExportKind> kind)
                           {
                               if (kind == MidiExportKind::pattern)
                                   exportActivePattern();
                               else if (kind == MidiExportKind::song)
                                   exportSong();
                           });
}

void MidiExportController::exportActivePattern()
{
    if (!canExportPattern())
        return;

    const auto pattern = model::Project{project_, nullptr}.pattern(activePattern_.index());
    view_.chooseMidiFileToSave("Pattern als MIDI exportieren",
                               pattern.name() + ".mid",
                               [this, tree = pattern.tree()](std::optional<std::filesystem::path> file)
                               {
                                   if (!file)
                                       return;
                                   const model::Project project{project_, nullptr};
                                   write(io::midiPatternOf(project, model::Pattern{tree, nullptr}),
                                         io::withMidiExtension(*file),
                                         "Das Pattern");
                               });
}

void MidiExportController::exportSong()
{
    if (!canExportSong())
        return;

    view_.chooseMidiFileToSave("Song als MIDI exportieren",
                               model::Project{project_, nullptr}.name() + ".mid",
                               [this](std::optional<std::filesystem::path> file)
                               {
                                   if (file)
                                       write(io::midiSongOf(model::Project{project_, nullptr}),
                                             io::withMidiExtension(*file),
                                             "Der Song");
                               });
}

void MidiExportController::write(const io::MidiPattern& midi,
                                 const std::filesystem::path& file,
                                 const std::string& what)
{
    if (!fileSystem_.writeText(file, io::writeMidiFile(midi)))
        view_.showMessage("Export fehlgeschlagen",
                          what + " konnte nicht als „" + io::utf8FromPath(file) + "“ gespeichert werden.");
}

} // namespace drumprog::ui
