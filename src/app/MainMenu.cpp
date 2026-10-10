#include "app/MainMenu.h"

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
constexpr juce::CommandID midiExportPattern = 0x7001;
constexpr juce::CommandID audioSettings = 0x3001;
constexpr juce::CommandID audioKeymap = 0x3002;
constexpr juce::CommandID audioLoadBackingTrack = 0x3003;
constexpr juce::CommandID audioRemoveBackingTrack = 0x3004;
constexpr juce::CommandID kitCopyToProject = 0x5001;
constexpr juce::CommandID kitUseGlobal = 0x5002;
constexpr juce::CommandID kitCopyToGlobal = 0x5003;
constexpr juce::CommandID transportRecord = 0x4001;
} // namespace command

constexpr int kCtrl = juce::ModifierKeys::commandModifier;
constexpr int kCtrlShift = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;

struct CommandSpec
{
    juce::CommandID id;
    const char* name; ///< UTF-8
    const char* description;
    const char* category;
    char key; ///< 0: no shortcut
    int modifiers;
};

constexpr std::array kCommands{
    CommandSpec{command::fileNew, "Neu", "Neues Projekt anlegen", "Datei", 'n', kCtrl},
    CommandSpec{command::fileOpen, "Öffnen...", "Projekt öffnen", "Datei", 'o', kCtrl},
    CommandSpec{command::fileSave, "Speichern", "Projekt speichern", "Datei", 's', kCtrl},
    CommandSpec{command::fileSaveAs,
                "Speichern unter...",
                "Projekt unter neuem Namen speichern",
                "Datei",
                's',
                kCtrlShift},
    CommandSpec{command::fileQuit, "Beenden", "Programm beenden", "Datei", 'q', kCtrl},
    CommandSpec{
        command::editUndo, "Rückgängig", "Letzte Änderung rückgängig machen", "Bearbeiten", 'z', kCtrl},
    CommandSpec{command::editRedo,
                "Wiederholen",
                "Rückgängig gemachte Änderung wiederholen",
                "Bearbeiten",
                'y',
                kCtrl},
    CommandSpec{command::editCut, "Ausschneiden", "Ausgewählte Noten ausschneiden", "Bearbeiten", 'x', kCtrl},
    CommandSpec{command::editCopy, "Kopieren", "Ausgewählte Noten kopieren", "Bearbeiten", 'c', kCtrl},
    CommandSpec{command::editPaste,
                "Einfügen",
                "Noten an der zuletzt geklickten Stelle einfügen",
                "Bearbeiten",
                'v',
                kCtrl},
    CommandSpec{command::editDuplicate,
                "Duplizieren",
                "Ausgewählte Noten dahinter kopieren",
                "Bearbeiten",
                'd',
                kCtrl},
    CommandSpec{command::editDelete, "Löschen", "Ausgewählte Noten löschen", "Bearbeiten", 0, 0},
    CommandSpec{command::editSelectAll,
                "Alles auswählen",
                "Alle Noten des Patterns auswählen",
                "Bearbeiten",
                'a',
                kCtrl},
    CommandSpec{command::toolDraw, "Werkzeug Zeichnen", "Noten zeichnen", "Bearbeiten", '1', kCtrl},
    CommandSpec{command::toolSelect, "Werkzeug Auswahl", "Noten auswählen", "Bearbeiten", '2', kCtrl},
    CommandSpec{command::toolErase, "Werkzeug Löschen", "Noten löschen", "Bearbeiten", '3', kCtrl},
    CommandSpec{command::patternNew, "Neu", "Neues Pattern anlegen", "Pattern", 0, 0},
    CommandSpec{command::patternDuplicate, "Duplizieren", "Aktives Pattern duplizieren", "Pattern", 0, 0},
    CommandSpec{command::patternRename, "Umbenennen...", "Aktives Pattern umbenennen", "Pattern", 0, 0},
    CommandSpec{command::patternLength, "Länge...", "Länge des aktiven Patterns in Takten", "Pattern", 0, 0},
    CommandSpec{command::patternDelete, "Löschen", "Aktives Pattern löschen", "Pattern", 0, 0},
    CommandSpec{
        command::songInsert, "Pattern einfügen", "Aktives Pattern ans Ende des Songs setzen", "Song", 0, 0},
    CommandSpec{command::songRemoveBlock, "Block löschen", "Ausgewählten Song-Block löschen", "Song", 0, 0},
    CommandSpec{command::songClear, "Song leeren", "Alle Blöcke aus dem Song entfernen", "Song", 0, 0},
    CommandSpec{command::midiExportPattern,
                "Export Pattern...",
                "Aktives Pattern als MIDI-Datei exportieren",
                "MIDI",
                'e',
                kCtrl},
    CommandSpec{command::audioSettings,
                "Einstellungen...",
                "Audio-Treiber, Gerät, Buffer und MIDI-Eingang wählen",
                "Audio",
                0,
                0},
    CommandSpec{command::audioKeymap, "Tastatur-Mapping...", "Tasten den Drum-Slots zuordnen", "Audio", 0, 0},
    CommandSpec{command::audioLoadBackingTrack,
                "Backing-Track laden...",
                "WAV-Datei als Backing-Track über der Drums-Spur importieren",
                "Audio",
                0,
                0},
    CommandSpec{command::audioRemoveBackingTrack,
                "Backing-Track entfernen",
                "Backing-Track aus dem Projekt entfernen",
                "Audio",
                0,
                0},
    CommandSpec{command::kitCopyToProject,
                "Kit ins Projekt übernehmen",
                "Das Projekt bekommt ein eigenes Kit, das das Programm-Kit überschreibt",
                "Kit",
                0,
                0},
    CommandSpec{command::kitUseGlobal,
                "Programm-Kit verwenden",
                "Das eigene Kit des Projekts entfernen",
                "Kit",
                0,
                0},
    CommandSpec{command::kitCopyToGlobal,
                "Projekt-Kit als Programm-Kit übernehmen",
                "Das Kit des Projekts wird das Kit für alle Projekte ohne eigenes Kit",
                "Kit",
                0,
                0},
    CommandSpec{command::transportRecord,
                "Aufnahme scharf",
                "Aufnahme scharf schalten oder entschärfen",
                "Transport",
                'r',
                kCtrl},
};

constexpr juce::CommandID kSeparator = 0;

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
        return {midiExportPattern};
    case 5:
        return {kitCopyToProject, kitUseGlobal, kSeparator, kitCopyToGlobal};
    default:
        return {audioSettings, audioKeymap, kSeparator, audioLoadBackingTrack, audioRemoveBackingTrack};
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
                   TransportActions transport)
    : document_(document), quit_(std::move(quit)), edit_(std::move(edit)), pattern_(std::move(pattern)),
      song_(std::move(song)), midi_(std::move(midi)), kit_(std::move(kit)), audio_(std::move(audio)),
      transport_(std::move(transport))
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
    return {"Datei", "Bearbeiten", "Pattern", "Song", "MIDI", "Kit", "Audio"};
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
    return menu;
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
        juce::String::fromUTF8(spec->name), juce::String::fromUTF8(spec->description), spec->category, 0);
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
    case command::midiExportPattern:
        return midi_.canExportPattern;
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
    case command::midiExportPattern:
        return midi_.exportPattern;
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
    default:
        return {};
    }
}

} // namespace drumprog::app
