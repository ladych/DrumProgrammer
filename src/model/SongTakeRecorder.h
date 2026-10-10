#pragma once

#include "model/IIdGenerator.h"
#include "model/Project.h"

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace drumprog::model
{

/// Writes a recording to the backing track, made in the song mode, into the song (F-BT-07, F-BT-08): the
/// hits of one pass become a new pattern "Take n" over the recorded bars, placed as a block on the drums
/// track where the recording started. Existing patterns stay as they are, and the whole take is one undo
/// step. The hits are collected while the take runs and written when it ends. GUI thread only.
class SongTakeRecorder
{
public:
    static constexpr const char* kUndoName = "Take";
    static constexpr const char* kNamePrefix = "Take ";

    SongTakeRecorder(juce::ValueTree project,
                     juce::ValueTree globalKit,
                     juce::UndoManager& undoManager,
                     IIdGenerator& idGenerator);

    /// Starts collecting hits; drops those of an earlier take that was not ended.
    void begin();
    /// Keeps a hit for the slot at slotIndex of the active kit at a song tick. Ignored without a take and
    /// for unknown slots.
    void add(int slotIndex, int velocity, std::int64_t songTick);
    /// Ends the take that started at startTick and stopped at stopTick. With hits it adds the pattern, from
    /// the bar of startTick to the end of the bar of stopTick or of the last hit, and its block; returns
    /// the index of the new pattern. Nothing without hits.
    std::optional<int> end(std::int64_t startTick, std::int64_t stopTick);
    [[nodiscard]] bool isRecording() const noexcept { return recording_; }

private:
    struct Hit
    {
        int slotNote = 0;
        int velocity = 0;
        std::int64_t tick = 0;
    };

    [[nodiscard]] std::string nextName() const;
    /// The hits as notes of the pattern, which starts at the song tick startTick.
    void writeNotes(Pattern& pattern, std::int64_t startTick, std::int64_t length) const;

    juce::ValueTree project_;
    juce::ValueTree globalKit_;
    juce::UndoManager& undoManager_;
    IIdGenerator& idGenerator_;
    std::vector<Hit> hits_;
    bool recording_ = false;
};

} // namespace drumprog::model
