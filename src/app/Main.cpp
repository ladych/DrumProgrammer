#include "app/AppComposition.h"
#include "app/Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace drumprog::app
{
namespace
{

/// Focus traversal of the main window without a default component. JUCE gives a click on a component
/// that does not want the keyboard focus to the first focusable component of its parent (e.g. the
/// pattern length field of the piano roll); that text field then took the drum keys. Now such a click
/// leaves the focus with the window, whose key listeners play the drums (Tab still reaches the fields).
class NoDefaultFocusTraverser final : public juce::KeyboardFocusTraverser
{
public:
    juce::Component* getDefaultComponent(juce::Component* /*parentComponent*/) override { return nullptr; }
};

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

    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override
    {
        return std::make_unique<NoDefaultFocusTraverser>();
    }

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
        juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel_);
        composition_ = std::make_unique<AppComposition>(lookAndFeel_);
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
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

    // Window close button and Strg+Q: ask for unsaved changes first (F-PJ-04).
    void systemRequestedQuit() override
    {
        composition_->documentController().requestClose([] { quit(); });
    }

private:
    theme::LookAndFeel lookAndFeel_;
    std::unique_ptr<AppComposition> composition_;
    std::unique_ptr<MainWindow> mainWindow_;
};

} // namespace
} // namespace drumprog::app

START_JUCE_APPLICATION(drumprog::app::DrumProgrammerApplication)
