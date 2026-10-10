#include "ui/MidiExportController.h"

#include "io/MidiFileWriter.h"
#include "io/PatternMidiExport.h"
#include "io/Utf8Path.h"
#include "model/Project.h"

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

void MidiExportController::exportActivePattern()
{
    if (!canExportPattern())
        return;

    const auto pattern = model::Project{project_, nullptr}.pattern(activePattern_.index());
    view_.chooseMidiFileToSave(pattern.name() + ".mid",
                               [this, tree = pattern.tree()](std::optional<std::filesystem::path> file)
                               {
                                   if (file)
                                       write(tree, io::withMidiExtension(*file));
                               });
}

void MidiExportController::write(const juce::ValueTree& pattern, const std::filesystem::path& file)
{
    const model::Project project{project_, nullptr};
    const auto midi = io::midiPatternOf(project, model::Pattern{pattern, nullptr});
    if (!fileSystem_.writeText(file, io::writeMidiFile(midi)))
        view_.showMessage("Export fehlgeschlagen",
                          "Das Pattern konnte nicht als „" + io::utf8FromPath(file) +
                              "“ gespeichert werden.");
}

} // namespace drumprog::ui
