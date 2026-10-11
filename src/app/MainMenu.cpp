#include "app/MainMenu.h"

#include "i18n/Translate.h"

#include <array>
#include <utility>
#include <vector>

namespace drumprog::app
{
namespace
{

namespace command
{
constexpr juce::CommandID fileNew = 0x1001;
constexpr juce::CommandID fileOpen = 0x1002;
constexpr juce::CommandID fileSave = 0x1003;
constexpr juce::CommandID fileSaveAs = 0x1004;
constexpr juce::CommandID fileQuit = 0x1005;
constexpr juce::CommandID editUndo = 0x2001;
constexpr juce::CommandID editRedo = 0x2002;
constexpr juce::CommandID editCut = 0x2003;
constexpr juce::CommandID editCopy = 0x2004;
constexpr juce::CommandID editPaste = 0x2005;
constexpr juce::CommandID editDuplicate = 0x2006;
constexpr juce::CommandID editDelete = 0x2007;
constexpr juce::CommandID editSelectAll = 0x2008;
constexpr juce::CommandID toolDraw = 0x2101;
constexpr juce::CommandID toolSelect = 0x2102;
constexpr juce::CommandID toolErase = 0x2103;
constexpr juce::CommandID patternNew = 0x6001;
constexpr juce::CommandID patternDuplicate = 0x6002;
constexpr juce::CommandID patternRename = 0x6003;
constexpr juce::CommandID patternLength = 0x6004;
constexpr juce::CommandID patternDelete = 0x6005;
constexpr juce::CommandID songInsert = 0x8001;
constexpr juce::CommandID songRemoveBlock = 0x8002;
constexpr juce::CommandID songClear = 0x8003;
constexpr juce::CommandID midiExport = 0x7001;
constexpr juce::CommandID midiImport = 0x7002;
constexpr juce::CommandID audioSettings = 0x3001;
constexpr juce::CommandID audioKeymap = 0x3002;
constexpr juce::CommandID audioLoadBackingTrack = 0x3003;
constexpr juce::CommandID audioRemoveBackingTrack = 0x3004;
constexpr juce::CommandID kitCopyToProject = 0x5001;
constexpr juce::CommandID kitUseGlobal = 0x5002;
constexpr juce::CommandID kitCopyToGlobal = 0x5003;
constexpr juce::CommandID kitShow = 0x5004;
constexpr juce::CommandID kitResetToDefault = 0x5005;
constexpr juce::CommandID transportRecord = 0x4001;
constexpr juce::CommandID helpAbout = 0x9001;
} // namespace command

constexpr int kCtrl = juce::ModifierKeys::commandModifier;
constexpr int kCtrlShift = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;

struct CommandSpec
{
    juce::CommandID id;
    const char* name; ///< English, translated when shown
    const char* description;
    const char* category;
    char key; ///< 0: no shortcut
    int modifiers;
};

constexpr std::array kCommands{
    CommandSpec{command::fileNew,
                i18n::marked("New"),
                i18n::marked("Create a new project"),
                i18n::marked("File"),
                'n',
                kCtrl},
    CommandSpec{command::fileOpen,
                i18n::marked("Open..."),
                i18n::marked("Open project"),
                i18n::marked("File"),
                'o',
                kCtrl},
    CommandSpec{command::fileSave,
                i18n::marked("Save"),
                i18n::marked("Save project"),
                i18n::marked("File"),
                's',
                kCtrl},
    CommandSpec{command::fileSaveAs,
                i18n::marked("Save as..."),
                i18n::marked("Save the project under a new name"),
                i18n::marked("File"),
                's',
                kCtrlShift},
    CommandSpec{command::fileQuit,
                i18n::marked("Quit"),
                i18n::marked("Quit the program"),
                i18n::marked("File"),
                'q',
                kCtrl},
    CommandSpec{command::editUndo,
                i18n::marked("Undo"),
                i18n::marked("Undo the last change"),
                i18n::marked("Edit"),
                'z',
                kCtrl},
    CommandSpec{command::editRedo,
                i18n::marked("Redo"),
                i18n::marked("Redo the undone change"),
                i18n::marked("Edit"),
                'y',
                kCtrl},
    CommandSpec{command::editCut,
                i18n::marked("Cut"),
                i18n::marked("Cut the selected notes"),
                i18n::marked("Edit"),
                'x',
                kCtrl},
    CommandSpec{command::editCopy,
                i18n::marked("Copy"),
                i18n::marked("Copy the selected notes"),
                i18n::marked("Edit"),
                'c',
                kCtrl},
    CommandSpec{command::editPaste,
                i18n::marked("Paste"),
                i18n::marked("Paste notes where you last clicked"),
                i18n::marked("Edit"),
                'v',
                kCtrl},
    CommandSpec{command::editDuplicate,
                i18n::marked("Duplicate"),
                i18n::marked("Copy the selected notes behind them"),
                i18n::marked("Edit"),
                'd',
                kCtrl},
    CommandSpec{command::editDelete,
                i18n::marked("Delete"),
                i18n::marked("Delete the selected notes"),
                i18n::marked("Edit"),
                0,
                0},
    CommandSpec{command::editSelectAll,
                i18n::marked("Select all"),
                i18n::marked("Select all notes of the pattern"),
                i18n::marked("Edit"),
                'a',
                kCtrl},
    CommandSpec{command::toolDraw,
                i18n::marked("Draw tool"),
                i18n::marked("Draw notes"),
                i18n::marked("Edit"),
                '1',
                kCtrl},
    CommandSpec{command::toolSelect,
                i18n::marked("Select tool"),
                i18n::marked("Select notes"),
                i18n::marked("Edit"),
                '2',
                kCtrl},
    CommandSpec{command::toolErase,
                i18n::marked("Erase tool"),
                i18n::marked("Delete notes"),
                i18n::marked("Edit"),
                '3',
                kCtrl},
    CommandSpec{command::patternNew,
                i18n::marked("New..."),
                i18n::marked("Create a new pattern"),
                i18n::marked("Pattern"),
                0,
                0},
    CommandSpec{command::patternDuplicate,
                i18n::marked("Duplicate"),
                i18n::marked("Duplicate the active pattern"),
                i18n::marked("Pattern"),
                0,
                0},
    CommandSpec{command::patternRename,
                i18n::marked("Rename..."),
                i18n::marked("Rename the active pattern"),
                i18n::marked("Pattern"),
                0,
                0},
    CommandSpec{command::patternLength,
                i18n::marked("Length..."),
                i18n::marked("Length of the active pattern in bars"),
                i18n::marked("Pattern"),
                0,
                0},
    CommandSpec{command::patternDelete,
                i18n::marked("Delete"),
                i18n::marked("Delete the active pattern"),
                i18n::marked("Pattern"),
                0,
                0},
    CommandSpec{command::songInsert,
                i18n::marked("Insert pattern"),
                i18n::marked("Put the active pattern at the end of the song"),
                i18n::marked("Song"),
                0,
                0},
    CommandSpec{command::songRemoveBlock,
                i18n::marked("Delete block"),
                i18n::marked("Delete the selected song block"),
                i18n::marked("Song"),
                0,
                0},
    CommandSpec{command::songClear,
                i18n::marked("Clear song"),
                i18n::marked("Remove all blocks from the song"),
                i18n::marked("Song"),
                0,
                0},
    CommandSpec{command::midiImport,
                i18n::marked("Import..."),
                i18n::marked("Import a MIDI file as a new pattern"),
                i18n::marked("MIDI"),
                'i',
                kCtrl},
    CommandSpec{command::midiExport,
                i18n::marked("Export..."),
                i18n::marked("Export the active pattern or the song as a MIDI file"),
                i18n::marked("MIDI"),
                'e',
                kCtrl},
    CommandSpec{command::audioSettings,
                i18n::marked("Settings..."),
                i18n::marked("Choose the audio driver, device, buffer and MIDI input"),
                i18n::marked("Audio"),
                0,
                0},
    CommandSpec{command::audioKeymap,
                i18n::marked("Keyboard mapping..."),
                i18n::marked("Assign keys to the drum slots"),
                i18n::marked("Audio"),
                0,
                0},
    CommandSpec{command::audioLoadBackingTrack,
                i18n::marked("Load backing track..."),
                i18n::marked("Import a WAV file as the backing track above the drums track"),
                i18n::marked("Audio"),
                0,
                0},
    CommandSpec{command::audioRemoveBackingTrack,
                i18n::marked("Remove backing track"),
                i18n::marked("Remove the backing track from the project"),
                i18n::marked("Audio"),
                0,
                0},
    CommandSpec{command::kitShow,
                i18n::marked("Edit kit..."),
                i18n::marked("Set the samples, volume, pitch, MIDI note and key of the slots"),
                i18n::marked("Kit"),
                'k',
                kCtrl},
    CommandSpec{command::kitCopyToProject,
                i18n::marked("Copy kit into the project"),
                i18n::marked("The project gets its own kit, which overrides the program kit"),
                i18n::marked("Kit"),
                0,
                0},
    CommandSpec{command::kitUseGlobal,
                i18n::marked("Use program kit"),
                i18n::marked("Remove the project's own kit"),
                i18n::marked("Kit"),
                0,
                0},
    CommandSpec{command::kitCopyToGlobal,
                i18n::marked("Make project kit the program kit"),
                i18n::marked("The project's kit becomes the kit of all projects without their own kit"),
                i18n::marked("Kit"),
                0,
                0},
    CommandSpec{command::kitResetToDefault,
                i18n::marked("Reset to default"),
                i18n::marked("Reset the shown kit to the default kit with its samples"),
                i18n::marked("Kit"),
                0,
                0},
    CommandSpec{command::helpAbout,
                i18n::marked("About and licenses..."),
                i18n::marked("Version, license, source code and third-party components"),
                i18n::marked("Help"),
                0,
                0},
    CommandSpec{command::transportRecord,
                i18n::marked("Arm recording"),
                i18n::marked("Arm or disarm recording"),
                i18n::marked("Transport"),
                'r',
                kCtrl},
};

constexpr juce::CommandID kSeparator = 0;
constexpr int kHelpMenu = 7;
/// Item IDs of the submenu "Sprache", outside the command IDs.
constexpr int kFirstLanguageItem = 0x9100;

std::vector<juce::CommandID> editMenuCommands()
{
    using namespace command;
    return {editUndo,
            editRedo,
            kSeparator,
            editCut,
            editCopy,
            editPaste,
            editDuplicate,
            editDelete,
            editSelectAll,
            kSeparator,
            toolDraw,
            toolSelect,
            toolErase};
}

/// Entries of the menus in the order of getMenuBarNames(); kSeparator draws a line.
std::vector<juce::CommandID> menuCommands(int menuIndex)
{
    using namespace command;
    switch (menuIndex)
    {
    case 0:
        return {fileNew, fileOpen, kSeparator, fileSave, fileSaveAs, kSeparator, fileQuit};
    case 1:
        return editMenuCommands();
    case 2:
        return {patternNew, patternDuplicate, patternRename, patternLength, patternDelete};
    case 3:
        return {songInsert, songRemoveBlock, kSeparator, songClear};
    case 4:
        return {midiImport, midiExport};
    case 5:
        return {kitShow,
                kSeparator,
                kitCopyToProject,
                kitUseGlobal,
                kSeparator,
                kitCopyToGlobal,
                kSeparator,
                kitResetToDefault};
    case 6:
        return {audioSettings, audioKeymap, kSeparator, audioLoadBackingTrack, audioRemoveBackingTrack};
    default:
        return {helpAbout};
    }
}

const CommandSpec* findCommand(juce::CommandID id)
{
    for (const auto& spec : kCommands)
        if (spec.id == id)
            return &spec;
    return nullptr;
}

} // namespace

MainMenu::MainMenu(ui::DocumentController& document,
                   std::function<void()> quit,
                   EditActions edit,
                   PatternActions pattern,
                   SongActions song,
                   MidiActions midi,
                   KitMenuActions kit,
                   AudioMenuActions audio,
                   HelpActions help,
                   TransportActions transport)
    : document_(document), quit_(std::move(quit)), edit_(std::move(edit)), pattern_(std::move(pattern)),
      song_(std::move(song)), midi_(std::move(midi)), kit_(std::move(kit)), audio_(std::move(audio)),
      help_(std::move(help)), transport_(std::move(transport))
{
    commands_.registerAllCommandsForTarget(this);
    commands_.setFirstCommandTarget(this);
    setApplicationCommandManagerToWatch(&commands_);
}

MainMenu::~MainMenu()
{
    setApplicationCommandManagerToWatch(nullptr);
}

juce::StringArray MainMenu::getMenuBarNames()
{
    juce::StringArray names;
    for (const auto* name : {i18n::marked("File"),
                             i18n::marked("Edit"),
                             i18n::marked("Pattern"),
                             i18n::marked("Song"),
                             i18n::marked("MIDI"),
                             i18n::marked("Kit"),
                             i18n::marked("Audio"),
                             i18n::marked("Help")})
        names.add(i18n::trJuce(name));
    return names;
}

juce::PopupMenu MainMenu::getMenuForIndex(int menuIndex, const juce::String& /*menuName*/)
{
    juce::PopupMenu menu;
    for (const auto id : menuCommands(menuIndex))
    {
        if (id == kSeparator)
            menu.addSeparator();
        else
            menu.addCommandItem(&commands_, id);
    }
    if (menuIndex == kHelpMenu)
    {
        juce::PopupMenu languages;
        int itemId = kFirstLanguageItem;
        for (const auto& choice : help_.languages())
            languages.addItem(itemId++, juce::String::fromUTF8(choice.label.c_str()), true, choice.ticked);
        menu.addSeparator();
        menu.addSubMenu(i18n::trJuce("Language"), languages);
    }
    return menu;
}

void MainMenu::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    if (topLevelMenuIndex != kHelpMenu || menuItemID < kFirstLanguageItem)
        return;
    const auto choices = help_.languages();
    const auto index = static_cast<std::size_t>(menuItemID - kFirstLanguageItem);
    if (index < choices.size())
        help_.chooseLanguage(choices[index].setting);
}

void MainMenu::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    for (const auto& spec : kCommands)
        commands.add(spec.id);
}

void MainMenu::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    const auto* spec = findCommand(commandID);
    if (spec == nullptr)
        return;

    result.setInfo(
        i18n::trJuce(spec->name), i18n::trJuce(spec->description), i18n::trJuce(spec->category), 0);
    if (spec->key != 0)
        result.addDefaultKeypress(spec->key, spec->modifiers);
    if (commandID == command::editRedo)
        result.addDefaultKeypress('z', kCtrlShift);
    if (commandID == command::editDelete)
        result.addDefaultKeypress(juce::KeyPress::deleteKey, 0);
    if (const auto isActive = activeCheckFor(commandID))
        result.setActive(isActive());
}

std::function<bool()> MainMenu::activeCheckFor(juce::CommandID commandID) const
{
    switch (commandID)
    {
    case command::editUndo:
        return [this] { return document_.canUndo(); };
    case command::editRedo:
        return [this] { return document_.canRedo(); };
    case command::editCut:
    case command::editCopy:
    case command::editDuplicate:
    case command::editDelete:
        return edit_.hasSelection;
    case command::editPaste:
        return edit_.canPaste;
    default:
        return projectCheckFor(commandID);
    }
}

std::function<bool()> MainMenu::projectCheckFor(juce::CommandID commandID) const
{
    switch (commandID)
    {
    case command::patternDelete:
        return pattern_.canRemove;
    case command::songRemoveBlock:
        return song_.hasSelection;
    case command::songClear:
        return [this] { return !song_.isEmpty(); };
    case command::midiExport:
        return midi_.canExport;
    case command::kitCopyToProject:
        return [this] { return !kit_.usesProjectKit(); };
    case command::kitUseGlobal:
    case command::kitCopyToGlobal:
        return kit_.usesProjectKit;
    case command::audioRemoveBackingTrack:
        return audio_.hasBackingTrack;
    default:
        return {};
    }
}

bool MainMenu::perform(const InvocationInfo& info)
{
    const auto action = actionFor(info.commandID);
    if (!action)
        return false;
    action();
    return true;
}

std::function<void()> MainMenu::actionFor(juce::CommandID commandID)
{
    switch (commandID)
    {
    case command::fileNew:
        return [this] { document_.newProject(); };
    case command::fileOpen:
        return [this] { document_.open(); };
    case command::fileSave:
        return [this] { document_.save(); };
    case command::fileSaveAs:
        return [this] { document_.saveAs(); };
    case command::fileQuit:
        return quit_;
    case command::editUndo:
        return [this] { document_.undo(); };
    case command::editRedo:
        return [this] { document_.redo(); };
    case command::transportRecord:
        return transport_.toggleRecordArmed;
    case command::midiImport:
        return midi_.importFile;
    case command::midiExport:
        return midi_.exportFile;
    default:
        return editActionFor(commandID);
    }
}

std::function<void()> MainMenu::editActionFor(juce::CommandID commandID) const
{
    switch (commandID)
    {
    case command::editCut:
        return edit_.cut;
    case command::editCopy:
        return edit_.copy;
    case command::editPaste:
        return edit_.paste;
    case command::editDuplicate:
        return edit_.duplicate;
    case command::editDelete:
        return edit_.deleteSelection;
    case command::editSelectAll:
        return edit_.selectAll;
    case command::toolDraw:
        return edit_.drawTool;
    case command::toolSelect:
        return edit_.selectTool;
    case command::toolErase:
        return edit_.eraseTool;
    default:
        return patternActionFor(commandID);
    }
}

std::function<void()> MainMenu::patternActionFor(juce::CommandID commandID) const
{
    switch (commandID)
    {
    case command::patternNew:
        return pattern_.add;
    case command::patternDuplicate:
        return pattern_.duplicate;
    case command::patternRename:
        return pattern_.rename;
    case command::patternLength:
        return pattern_.setLength;
    case command::patternDelete:
        return pattern_.remove;
    default:
        return songActionFor(commandID);
    }
}

std::function<void()> MainMenu::songActionFor(juce::CommandID commandID) const
{
    switch (commandID)
    {
    case command::songInsert:
        return song_.insertPattern;
    case command::songRemoveBlock:
        return song_.removeBlock;
    case command::songClear:
        return song_.clear;
    default:
        return kitActionFor(commandID);
    }
}

std::function<void()> MainMenu::kitActionFor(juce::CommandID commandID) const
{
    switch (commandID)
    {
    case command::kitShow:
        return kit_.showKit;
    case command::kitResetToDefault:
        return kit_.resetToDefault;
    case command::kitCopyToProject:
        return kit_.copyGlobalKitToProject;
    case command::kitUseGlobal:
        return kit_.useGlobalKit;
    case command::kitCopyToGlobal:
        return kit_.copyProjectKitToGlobal;
    default:
        return audioActionFor(commandID);
    }
}

std::function<void()> MainMenu::audioActionFor(juce::CommandID commandID) const
{
    switch (commandID)
    {
    case command::audioSettings:
        return audio_.showSettings;
    case command::audioKeymap:
        return audio_.showKeymap;
    case command::audioLoadBackingTrack:
        return audio_.loadBackingTrack;
    case command::audioRemoveBackingTrack:
        return audio_.removeBackingTrack;
    case command::helpAbout:
        return help_.showAbout;
    default:
        return {};
    }
}

} // namespace drumprog::app
