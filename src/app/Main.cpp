#include "app/AppComposition.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace drumprog::app
{
namespace
{

class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow(const juce::String& name,
               std::unique_ptr<juce::Component> content,
               MainMenu& menu,
               juce::KeyListener& drumKeys)
        : DocumentWindow(name,
                         juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                             juce::ResizableWindow::backgroundColourId),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setMenuBar(&menu);
        addKeyListener(menu.keyMappings());
        addKeyListener(&drumKeys);
        setContentOwned(content.release(), true);
        setResizable(true, true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    ~MainWindow() override { setMenuBar(nullptr); }

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};

class DrumProgrammerApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String& /*commandLine*/) override
    {
        composition_ = std::make_unique<AppComposition>();
        mainWindow_ = std::make_unique<MainWindow>(getApplicationName(),
                                                   composition_->createMainComponent(),
                                                   composition_->mainMenu(),
                                                   composition_->keyboardListener());
        composition_->setDialogParent(mainWindow_.get());
        composition_->documentView().setTitleSink([this](const juce::String& title)
                                                  { mainWindow_->setName(title); });
    }

    void shutdown() override
    {
        mainWindow_.reset();
        composition_.reset();
    }

    // Window close button and Strg+Q: ask for unsaved changes first (F-PJ-04).
    void systemRequestedQuit() override
    {
        composition_->documentController().requestClose([] { quit(); });
    }

private:
    std::unique_ptr<AppComposition> composition_;
    std::unique_ptr<MainWindow> mainWindow_;
};

} // namespace
} // namespace drumprog::app

START_JUCE_APPLICATION(drumprog::app::DrumProgrammerApplication)
