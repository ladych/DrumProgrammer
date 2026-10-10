#include "app/JuceMidiExportView.h"

#include "io/Utf8Path.h"

#include <utility>

namespace drumprog::app
{
namespace
{

juce::String fromUtf8(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

} // namespace

void JuceMidiExportView::chooseMidiFileToSave(const std::string& suggestedFileName,
                                              ui::FileChosenCallback onChosen)
{
    const auto start = directory_.getChildFile(juce::File::createLegalFileName(fromUtf8(suggestedFileName)));
    // The chooser must outlive the asynchronous dialog, so it is kept until the next one.
    chooser_ = std::make_unique<juce::FileChooser>("Pattern als MIDI exportieren", start, "*.mid;*.midi");
    chooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                              juce::FileBrowserComponent::warnAboutOverwriting,
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

void JuceMidiExportView::showMessage(const std::string& title, const std::string& message)
{
    juce::AlertWindow::showAsync(juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                                        fromUtf8(title),
                                                                        fromUtf8(message)),
                                 nullptr);
}

} // namespace drumprog::app
