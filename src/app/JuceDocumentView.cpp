#include "app/JuceDocumentView.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"

#include <utility>

namespace drumprog::app
{
namespace
{

constexpr const char* kProjectFilePattern = "*.dpp";

juce::String fromUtf8(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

// AlertWindow button indices for makeOptionsYesNoCancel: first = 1, second = 2, third = 0.
constexpr int kSaveButton = 1;
constexpr int kDiscardButton = 2;

ui::SaveChangesChoice choiceForButton(int button)
{
    if (button == kSaveButton)
        return ui::SaveChangesChoice::save;
    if (button == kDiscardButton)
        return ui::SaveChangesChoice::discard;
    return ui::SaveChangesChoice::cancel;
}

} // namespace

void JuceDocumentView::setTitleSink(TitleSink sink)
{
    titleSink_ = std::move(sink);
    titleSink_(title_);
}

void JuceDocumentView::setWindowTitle(const std::string& title)
{
    title_ = fromUtf8(title);
    if (titleSink_)
        titleSink_(title_);
}

void JuceDocumentView::askToSaveChanges(const std::string& projectName,
                                        std::function<void(ui::SaveChangesChoice)> onChoice)
{
    const auto options =
        juce::MessageBoxOptions::makeOptionsYesNoCancel(juce::MessageBoxIconType::QuestionIcon,
                                                        i18n::trJuce("Unsaved changes"),
                                                        i18n::trJuce("Save changes to \"%1\"?", projectName),
                                                        i18n::trJuce("Save"),
                                                        i18n::trJuce("Discard"),
                                                        i18n::trJuce("Cancel"));
    juce::AlertWindow::showAsync(
        options, [onChoice = std::move(onChoice)](int button) { onChoice(choiceForButton(button)); });
}

void JuceDocumentView::chooseFileToOpen(ui::FileChosenCallback onChosen)
{
    launchChooser(i18n::trJuce("Open project"),
                  juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                  juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                  std::move(onChosen));
}

void JuceDocumentView::chooseFileToSave(const std::string& suggestedFileName, ui::FileChosenCallback onChosen)
{
    launchChooser(i18n::trJuce("Save project"),
                  juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                      .getChildFile(fromUtf8(suggestedFileName)),
                  juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                      juce::FileBrowserComponent::warnAboutOverwriting,
                  std::move(onChosen));
}

void JuceDocumentView::askToRestoreBackup(std::function<void(bool restore)> onChoice)
{
    const auto options = juce::MessageBoxOptions::makeOptionsOkCancel(
        juce::MessageBoxIconType::QuestionIcon,
        i18n::trJuce("Restore unsaved changes"),
        i18n::trJuce(
            "Drum Programmer was not closed properly. Restore the unsaved changes from the automatic "
            "backup?"),
        i18n::trJuce("Restore"),
        i18n::trJuce("Discard"));
    juce::AlertWindow::showAsync(options,
                                 [onChoice = std::move(onChoice)](int button) { onChoice(button == 1); });
}

void JuceDocumentView::showMessage(const std::string& title, const std::string& message)
{
    juce::AlertWindow::showAsync(juce::MessageBoxOptions::makeOptionsOk(
                                     juce::MessageBoxIconType::InfoIcon, fromUtf8(title), fromUtf8(message)),
                                 nullptr);
}

void JuceDocumentView::launchChooser(const juce::String& title,
                                     const juce::File& start,
                                     int flags,
                                     ui::FileChosenCallback onChosen)
{
    // The chooser must outlive the asynchronous dialog, so it is kept until the next one.
    chooser_ = std::make_unique<juce::FileChooser>(title, start, kProjectFilePattern);
    chooser_->launchAsync(flags,
                          [onChosen = std::move(onChosen)](const juce::FileChooser& chooser)
                          {
                              const auto file = chooser.getResult();
                              if (file == juce::File{})
                                  onChosen(std::nullopt);
                              else
                                  onChosen(io::pathFromUtf8(file.getFullPathName().toStdString()));
                          });
}

} // namespace drumprog::app
