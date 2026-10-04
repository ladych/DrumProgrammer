#pragma once

#include "model/TreeNode.h"

#include <cstdint>
#include <optional>
#include <string>

namespace drumprog::model
{

/// Ticks per quarter note (Pflichtenheft chapter 4): 1/16 = 240 ticks, 1/32 triplet = 80 ticks.
inline constexpr int kTicksPerQuarter = 960;

struct TimeSignature
{
    int numerator = 4;
    int denominator = 4;
};

/// grid = drawn on the grid (orange), live = played in unquantised (violet).
enum class NoteOrigin : std::uint8_t
{
    grid,
    live
};

struct NoteData
{
    int slotNote = 0; ///< gmNote of the slot the note plays
    std::int64_t startTick = 0;
    std::int64_t lengthTicks = 0;
    int velocity = 100; ///< 1-127
    NoteOrigin origin = NoteOrigin::grid;
};

class Note : public TreeNode
{
public:
    Note(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] NoteData data() const;
    [[nodiscard]] int slotNote() const;
    [[nodiscard]] std::int64_t startTick() const;
    [[nodiscard]] std::int64_t lengthTicks() const;
    [[nodiscard]] int velocity() const;
    [[nodiscard]] NoteOrigin origin() const;

    void setSlotNote(int slotNote);
    void setStartTick(std::int64_t startTick);
    void setLengthTicks(std::int64_t lengthTicks);
    /// Clamped to 1-127.
    void setVelocity(int velocity);
    void setOrigin(NoteOrigin origin);
};

class Pattern : public TreeNode
{
public:
    Pattern(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] std::string id() const;
    [[nodiscard]] std::string name() const;
    [[nodiscard]] std::string colour() const;
    [[nodiscard]] int lengthBars() const;
    [[nodiscard]] int numNotes() const;
    [[nodiscard]] Note note(int index) const;

    void setName(const std::string& name);
    /// Colour as "#RRGGBB".
    void setColour(const std::string& colour);
    /// At least one bar.
    void setLengthBars(int lengthBars);
    Note addNote(const NoteData& data);
    void removeNote(int index);
};

/// One sample slot of the kit. gmNote identifies the slot and never changes; midiNote is the
/// (overridable) note that triggers it.
class SampleSlot : public TreeNode
{
public:
    SampleSlot(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] int gmNote() const;
    [[nodiscard]] int midiNote() const;
    [[nodiscard]] std::string name() const;
    /// Absolute path in memory; the .dpp file stores it relative to the project file.
    [[nodiscard]] std::string filePath() const;
    [[nodiscard]] double gain() const;
    [[nodiscard]] double pitch() const;
    [[nodiscard]] int chokeGroup() const;
    /// True if the file was missing when the project was loaded (F-PJ-03).
    [[nodiscard]] bool isSampleMissing() const;
    /// False shows the slot as "kein Sample".
    [[nodiscard]] bool hasSample() const;

    void setMidiNote(int midiNote);
    /// Assigning a file clears the missing marker.
    void setFilePath(const std::string& filePath);
    void setGain(double gain);
    void setPitch(double semitones);
    void setChokeGroup(int chokeGroup);
};

class Kit : public TreeNode
{
public:
    Kit(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] int numSlots() const;
    [[nodiscard]] SampleSlot slot(int index) const;
    [[nodiscard]] std::optional<SampleSlot> findSlot(int gmNote) const;
};

class SongEntry : public TreeNode
{
public:
    SongEntry(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] std::string patternId() const;
    [[nodiscard]] int startBar() const;
    void setStartBar(int startBar);
};

/// Song timeline; repeated blocks reference the same pattern id.
class Song : public TreeNode
{
public:
    Song(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] int numEntries() const;
    [[nodiscard]] SongEntry entry(int index) const;
    SongEntry addEntry(const std::string& patternId, int startBar);
    void removeEntry(int index);
    void removeEntriesFor(const std::string& patternId);
};

class BackingTrack : public TreeNode
{
public:
    BackingTrack(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] std::string filePath() const;
    [[nodiscard]] std::int64_t offsetSamples() const;
    [[nodiscard]] double gain() const;

    void setFilePath(const std::string& filePath);
    void setOffsetSamples(std::int64_t offsetSamples);
    void setGain(double gain);
};

/// The three reference faders "Mix (Referenz)".
class Mix : public TreeNode
{
public:
    Mix(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] double backingGain() const;
    [[nodiscard]] double drumsGain() const;
    [[nodiscard]] double masterGain() const;

    void setBackingGain(double gain);
    void setDrumsGain(double gain);
    void setMasterGain(double gain);
};

/// Root of the model, saved as .dpp file.
class Project : public TreeNode
{
public:
    Project(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] std::string name() const;
    [[nodiscard]] double bpm() const;
    [[nodiscard]] TimeSignature timeSignature() const;
    [[nodiscard]] int ticksPerQuarter() const;
    [[nodiscard]] std::int64_t ticksPerBar() const;

    void setName(const std::string& name);
    void setBpm(double bpm);
    void setTimeSignature(TimeSignature timeSignature);

    /// The project's own kit; its tree is invalid (no slots) while the project uses the global kit.
    [[nodiscard]] Kit kit() const;
    /// True if the project brings its own kit, which then overrides the global kit.
    [[nodiscard]] bool hasOwnKit() const;
    /// The kit the project plays with: its own kit, otherwise the global kit (program setting).
    [[nodiscard]] Kit activeKit(const juce::ValueTree& globalKit) const;
    /// Gives the project a copy of the kit as its own kit, replacing an own kit it had.
    void setOwnKit(const juce::ValueTree& kit);
    /// Removes the project's own kit, so it uses the global kit again.
    void removeOwnKit();
    [[nodiscard]] Song song() const;
    [[nodiscard]] BackingTrack backingTrack() const;
    [[nodiscard]] Mix mix() const;

    [[nodiscard]] int numPatterns() const;
    [[nodiscard]] Pattern pattern(int index) const;
    [[nodiscard]] std::optional<Pattern> findPattern(const std::string& id) const;
    Pattern addPattern(const std::string& id, const std::string& name, int lengthBars);
    /// Also removes all song blocks that use the pattern (F-SO-05).
    void removePattern(int index);

private:
    [[nodiscard]] juce::ValueTree child(const juce::Identifier& type) const;
};

} // namespace drumprog::model
