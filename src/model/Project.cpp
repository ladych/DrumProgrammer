#include "model/Project.h"

#include "model/ModelIds.h"

#include <algorithm>
#include <utility>

namespace drumprog::model
{
namespace
{

constexpr int kMinVelocity = 1;
constexpr int kMaxVelocity = 127;
constexpr const char* kDefaultPatternColour = "#E8743B";
constexpr const char* kOriginLive = "live";
constexpr const char* kOriginGrid = "grid";
constexpr int kQuartersPerWholeNote = 4;

juce::String toUtf8String(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

} // namespace

// ----- Note ---------------------------------------------------------------------------------------

Note::Note(juce::ValueTree tree, juce::UndoManager* undoManager) : TreeNode(std::move(tree), undoManager) {}

NoteData Note::data() const
{
    return {slotNote(), startTick(), lengthTicks(), velocity(), origin()};
}

int Note::slotNote() const
{
    return getInt(ids::slotNote, 0);
}
std::int64_t Note::startTick() const
{
    return getInt64(ids::startTick, 0);
}
std::int64_t Note::lengthTicks() const
{
    return getInt64(ids::lengthTicks, 0);
}
int Note::velocity() const
{
    return getInt(ids::velocity, kMaxVelocity);
}

NoteOrigin Note::origin() const
{
    return getString(ids::origin) == kOriginLive ? NoteOrigin::live : NoteOrigin::grid;
}

void Note::setSlotNote(int slotNote)
{
    set(ids::slotNote, slotNote);
}
void Note::setStartTick(std::int64_t startTick)
{
    set(ids::startTick, static_cast<juce::int64>(startTick));
}
void Note::setLengthTicks(std::int64_t lengthTicks)
{
    set(ids::lengthTicks, static_cast<juce::int64>(lengthTicks));
}
void Note::setVelocity(int velocity)
{
    set(ids::velocity, std::clamp(velocity, kMinVelocity, kMaxVelocity));
}

void Note::setOrigin(NoteOrigin origin)
{
    set(ids::origin, origin == NoteOrigin::live ? kOriginLive : kOriginGrid);
}

// ----- Pattern ------------------------------------------------------------------------------------

Pattern::Pattern(juce::ValueTree tree, juce::UndoManager* undoManager)
    : TreeNode(std::move(tree), undoManager)
{
}

std::string Pattern::id() const
{
    return getString(ids::id);
}
std::string Pattern::name() const
{
    return getString(ids::name);
}
std::string Pattern::colour() const
{
    return getString(ids::colour);
}
int Pattern::lengthBars() const
{
    return getInt(ids::lengthBars, 1);
}
int Pattern::numNotes() const
{
    return tree().getNumChildren();
}
Note Pattern::note(int index) const
{
    return {tree().getChild(index), undoManager()};
}

void Pattern::setName(const std::string& name)
{
    setString(ids::name, name);
}
void Pattern::setColour(const std::string& colour)
{
    setString(ids::colour, colour);
}
void Pattern::setLengthBars(int lengthBars)
{
    set(ids::lengthBars, std::max(lengthBars, 1));
}

Note Pattern::addNote(const NoteData& data)
{
    Note note{juce::ValueTree{ids::note}, undoManager()};
    note.setSlotNote(data.slotNote);
    note.setStartTick(data.startTick);
    note.setLengthTicks(data.lengthTicks);
    note.setVelocity(data.velocity);
    note.setOrigin(data.origin);
    mutableTree().appendChild(note.tree(), undoManager());
    return note;
}

void Pattern::removeNote(int index)
{
    mutableTree().removeChild(index, undoManager());
}

// ----- SampleSlot ---------------------------------------------------------------------------------

SampleSlot::SampleSlot(juce::ValueTree tree, juce::UndoManager* undoManager)
    : TreeNode(std::move(tree), undoManager)
{
}

int SampleSlot::gmNote() const
{
    return getInt(ids::gmNote, 0);
}
int SampleSlot::midiNote() const
{
    return getInt(ids::midiNote, gmNote());
}
std::string SampleSlot::name() const
{
    return getString(ids::name);
}
std::string SampleSlot::filePath() const
{
    return getString(ids::filePath);
}
double SampleSlot::gain() const
{
    return getDouble(ids::gain, 1.0);
}
double SampleSlot::pitch() const
{
    return getDouble(ids::pitch, 0.0);
}
int SampleSlot::chokeGroup() const
{
    return getInt(ids::chokeGroup, 0);
}
bool SampleSlot::isSampleMissing() const
{
    return static_cast<bool>(tree().getProperty(ids::sampleMissing));
}
bool SampleSlot::hasSample() const
{
    if (isSampleMissing())
        return false;
    return !filePath().empty();
}

void SampleSlot::setMidiNote(int midiNote)
{
    set(ids::midiNote, midiNote);
}

void SampleSlot::setFilePath(const std::string& filePath)
{
    setString(ids::filePath, filePath);
    mutableTree().removeProperty(ids::sampleMissing, undoManager());
}

void SampleSlot::setGain(double gain)
{
    set(ids::gain, gain);
}
void SampleSlot::setPitch(double semitones)
{
    set(ids::pitch, semitones);
}
void SampleSlot::setChokeGroup(int chokeGroup)
{
    set(ids::chokeGroup, chokeGroup);
}

// ----- Kit ----------------------------------------------------------------------------------------

Kit::Kit(juce::ValueTree tree, juce::UndoManager* undoManager) : TreeNode(std::move(tree), undoManager) {}

int Kit::numSlots() const
{
    return tree().getNumChildren();
}
SampleSlot Kit::slot(int index) const
{
    return {tree().getChild(index), undoManager()};
}

std::optional<SampleSlot> Kit::findSlot(int gmNote) const
{
    const auto slotTree = tree().getChildWithProperty(ids::gmNote, gmNote);
    if (!slotTree.isValid())
        return std::nullopt;
    return SampleSlot{slotTree, undoManager()};
}

// ----- Song ---------------------------------------------------------------------------------------

SongEntry::SongEntry(juce::ValueTree tree, juce::UndoManager* undoManager)
    : TreeNode(std::move(tree), undoManager)
{
}

std::string SongEntry::patternId() const
{
    return getString(ids::patternId);
}
int SongEntry::startBar() const
{
    return getInt(ids::startBar, 0);
}
void SongEntry::setStartBar(int startBar)
{
    set(ids::startBar, startBar);
}

Song::Song(juce::ValueTree tree, juce::UndoManager* undoManager) : TreeNode(std::move(tree), undoManager) {}

int Song::numEntries() const
{
    return tree().getNumChildren();
}
SongEntry Song::entry(int index) const
{
    return {tree().getChild(index), undoManager()};
}

SongEntry Song::addEntry(const std::string& patternId, int startBar)
{
    juce::ValueTree entryTree{ids::songEntry};
    entryTree.setProperty(ids::patternId, toUtf8String(patternId), nullptr);
    entryTree.setProperty(ids::startBar, startBar, nullptr);
    mutableTree().appendChild(entryTree, undoManager());
    return {entryTree, undoManager()};
}

void Song::removeEntry(int index)
{
    mutableTree().removeChild(index, undoManager());
}

void Song::removeEntriesFor(const std::string& patternId)
{
    for (int index = numEntries() - 1; index >= 0; --index)
        if (entry(index).patternId() == patternId)
            removeEntry(index);
}

// ----- BackingTrack and Mix -----------------------------------------------------------------------

BackingTrack::BackingTrack(juce::ValueTree tree, juce::UndoManager* undoManager)
    : TreeNode(std::move(tree), undoManager)
{
}

std::string BackingTrack::filePath() const
{
    return getString(ids::filePath);
}
std::int64_t BackingTrack::offsetSamples() const
{
    return getInt64(ids::offsetSamples, 0);
}
double BackingTrack::gain() const
{
    return getDouble(ids::gain, 1.0);
}

void BackingTrack::setFilePath(const std::string& filePath)
{
    setString(ids::filePath, filePath);
}

void BackingTrack::setOffsetSamples(std::int64_t offsetSamples)
{
    set(ids::offsetSamples, static_cast<juce::int64>(offsetSamples));
}

void BackingTrack::setGain(double gain)
{
    set(ids::gain, gain);
}

Mix::Mix(juce::ValueTree tree, juce::UndoManager* undoManager) : TreeNode(std::move(tree), undoManager) {}

double Mix::backingGain() const
{
    return getDouble(ids::backingGain, 1.0);
}
double Mix::drumsGain() const
{
    return getDouble(ids::drumsGain, 1.0);
}
double Mix::masterGain() const
{
    return getDouble(ids::masterGain, 1.0);
}
void Mix::setBackingGain(double gain)
{
    set(ids::backingGain, gain);
}
void Mix::setDrumsGain(double gain)
{
    set(ids::drumsGain, gain);
}
void Mix::setMasterGain(double gain)
{
    set(ids::masterGain, gain);
}

// ----- Project ------------------------------------------------------------------------------------

Project::Project(juce::ValueTree tree, juce::UndoManager* undoManager)
    : TreeNode(std::move(tree), undoManager)
{
}

std::string Project::name() const
{
    return getString(ids::name);
}
double Project::bpm() const
{
    return getDouble(ids::bpm, 0.0);
}

TimeSignature Project::timeSignature() const
{
    return {getInt(ids::timeSigNumerator, 4), getInt(ids::timeSigDenominator, 4)};
}

int Project::ticksPerQuarter() const
{
    return getInt(ids::ppq, kTicksPerQuarter);
}

std::int64_t Project::ticksPerBar() const
{
    const auto signature = timeSignature();
    return std::int64_t{ticksPerQuarter()} * kQuartersPerWholeNote * signature.numerator /
           signature.denominator;
}

void Project::setName(const std::string& name)
{
    setString(ids::name, name);
}
void Project::setBpm(double bpm)
{
    set(ids::bpm, bpm);
}

void Project::setTimeSignature(TimeSignature timeSignature)
{
    set(ids::timeSigNumerator, timeSignature.numerator);
    set(ids::timeSigDenominator, timeSignature.denominator);
}

Kit Project::kit() const
{
    return {child(ids::kit), undoManager()};
}

bool Project::hasOwnKit() const
{
    return child(ids::kit).isValid();
}

Kit Project::activeKit(const juce::ValueTree& globalKit) const
{
    if (hasOwnKit())
        return kit();
    return {globalKit, undoManager()};
}

void Project::setOwnKit(const juce::ValueTree& kit)
{
    removeOwnKit();
    mutableTree().appendChild(kit.createCopy(), undoManager());
}

void Project::removeOwnKit()
{
    if (hasOwnKit())
        mutableTree().removeChild(child(ids::kit), undoManager());
}
Song Project::song() const
{
    return {child(ids::song), undoManager()};
}
BackingTrack Project::backingTrack() const
{
    return {child(ids::backingTrack), undoManager()};
}
Mix Project::mix() const
{
    return {child(ids::mix), undoManager()};
}

int Project::numPatterns() const
{
    return child(ids::patterns).getNumChildren();
}
Pattern Project::pattern(int index) const
{
    return {child(ids::patterns).getChild(index), undoManager()};
}

std::optional<Pattern> Project::findPattern(const std::string& id) const
{
    const auto patternTree = child(ids::patterns).getChildWithProperty(ids::id, toUtf8String(id));
    if (!patternTree.isValid())
        return std::nullopt;
    return Pattern{patternTree, undoManager()};
}

Pattern Project::addPattern(const std::string& id, const std::string& name, int lengthBars)
{
    juce::ValueTree patternTree{ids::pattern};
    patternTree.setProperty(ids::id, toUtf8String(id), nullptr);
    patternTree.setProperty(ids::name, toUtf8String(name), nullptr);
    patternTree.setProperty(ids::colour, kDefaultPatternColour, nullptr);
    patternTree.setProperty(ids::lengthBars, std::max(lengthBars, 1), nullptr);
    child(ids::patterns).appendChild(patternTree, undoManager());
    return {patternTree, undoManager()};
}

void Project::removePattern(int index)
{
    song().removeEntriesFor(pattern(index).id());
    child(ids::patterns).removeChild(index, undoManager());
}

juce::ValueTree Project::child(const juce::Identifier& type) const
{
    return tree().getChildWithName(type);
}

} // namespace drumprog::model
