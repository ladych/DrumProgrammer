#include "app/JuceMidiDialogs.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"

#include <utility>
#include <vector>

namespace drumprog::app
{
namespace
{

constexpr const char* kMidiFiles = "*.mid;*.midi";

juce::String fromUtf8(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

} // namespace

void JuceMidiDialogs::chooseExportKind(bool canExportPattern,
                                       bool canExportSong,
                                       ui::ExportKindCallback onChosen)
{
    // The alert window returns 1 and 2 for the first buttons and 0 for the last one (cancel).
    std::vector<ui::MidiExportKind> kinds;
    auto options = juce::MessageBoxOptions()
                       .withIconType(juce::MessageBoxIconType::QuestionIcon)
                       .withTitle(i18n::trJuce("Export MIDI"))
                       .withMessage(i18n::trJuce("What should be exported as a MIDI file?"));
    if (canExportPattern)
    {
        options = options.withButton(i18n::trJuce("Active pattern"));
        kinds.push_back(ui::MidiExportKind::pattern);
    }
    if (canExportSong)
    {
        options = options.withButton(i18n::trJuce("Whole song"));
        kinds.push_back(ui::MidiExportKind::song);
    }
    options = options.withButton(i18n::trJuce("Cancel"));
    juce::AlertWindow::showAsync(options,
                                 [kinds, onChosen = std::move(onChosen)](int result)
                                 {
                                     if (result >= 1 && result <= static_cast<int>(kinds.size()))
                                         onChosen(kinds[static_cast<std::size_t>(result - 1)]);
                                     else
                                         onChosen(std::nullopt);
                                 });
}

void JuceMidiDialogs::chooseMidiFileToSave(const std::string& title,
                                           const std::string& suggestedFileName,
                                           ui::FileChosenCallback onChosen)
{
    launch(fromUtf8(title),
           directory_.getChildFile(juce::File::createLegalFileName(fromUtf8(suggestedFileName))),
           juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
               juce::FileBrowserComponent::warnAboutOverwriting,
           std::move(onChosen));
}

void JuceMidiDialogs::chooseMidiFileToOpen(ui::FileChosenCallback onChosen)
{
    launch(i18n::trJuce("Import MIDI file as a pattern"),
           directory_,
           juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
           [onChosen = std::move(onChosen)](std::optional<std::filesystem::path> file)
           {
               juce::MouseCursor::showWaitCursor();
               onChosen(std::move(file));
               juce::MouseCursor::hideWaitCursor();
           });
}

void JuceMidiDialogs::askToAdoptTempo(const std::string& question, std::function<void(bool)> onAnswer)
{
    juce::AlertWindow::showAsync(
        juce::MessageBoxOptions::makeOptionsOkCancel(juce::MessageBoxIconType::QuestionIcon,
                                                     i18n::trJuce("Tempo and time signature"),
                                                     fromUtf8(question),
                                                     i18n::trJuce("Adopt"),
                                                     i18n::trJuce("Keep")),
        [onAnswer = std::move(onAnswer)](int result)
        {
            juce::MouseCursor::showWaitCursor();
            onAnswer(result == 1);
            juce::MouseCursor::hideWaitCursor();
        });
}

void JuceMidiDialogs::launch(const juce::String& title,
                             const juce::File& start,
                             int flags,
                             ui::FileChosenCallback onChosen)
{
    // The chooser must outlive the asynchronous dialog, so it is kept until the next one.
    chooser_ = std::make_unique<juce::FileChooser>(title, start, kMidiFiles);
    chooser_->launchAsync(flags,
                          [this, onChosen = std::move(onChosen)](const juce::FileChooser& chooser)
                          {
                              const auto file = chooser.getResult();
                              if (file == juce::File{})
                              {
                                  onChosen(std::nullopt);
                                  return;
                              }
                              directory_ = file.getParentDirectory();
                              onChosen(io::pathFromUtf8(file.getFullPathName().toStdString()));
                          });
}

void JuceMidiDialogs::showMessage(const std::string& title, const std::string& message)
{
    juce::AlertWindow::showAsync(juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                                        fromUtf8(title),
                                                                        fromUtf8(message)),
                                 nullptr);
}

} // namespace drumprog::app
