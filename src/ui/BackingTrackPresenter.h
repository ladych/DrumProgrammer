#pragma once

#include "engine/BackingTrackPlayer.h"
#include "model/Project.h"
#include "model/TreeChangeListener.h"
#include "ui/IBackingTrackLoader.h"

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace drumprog::ui
{

/// Logic of the backing track lane of the song timeline (F-BT-01, F-BT-04, F-BT-05; humble view in
/// app/SongTimelineView) and of the menu entries "Backing-Track laden/entfernen". The model keeps the file
/// and the offset; this presenter opens the file whenever it changes, also by undo or loading, and hands
/// the stream to the BackingTrackPlayer. Positions on the timeline are in bars of the song, fractional.
/// GUI thread only.
class BackingTrackPresenter
{
public:
    BackingTrackPresenter(juce::ValueTree project,
                          juce::UndoManager& undoManager,
                          IBackingTrackLoader& loader,
                          engine::BackingTrackPlayer& player);

    /// Replaces the track with the file, offset 0, as one undo step; false if it cannot be read.
    bool load(const std::filesystem::path& file);
    /// One undo step; nothing without a track.
    void remove();

    /// True if the project has a backing track, even if its file is missing.
    [[nodiscard]] bool hasTrack() const;
    /// True if the project's file could not be opened.
    [[nodiscard]] bool isMissing() const;
    [[nodiscard]] std::filesystem::path file() const;
    /// File name, with a note if the file is missing; empty without a track.
    [[nodiscard]] std::string label() const;
    [[nodiscard]] double lengthSeconds() const;

    /// Bar of the song where the file starts and ends; both 0 without a playable file.
    [[nodiscard]] double startBar() const;
    [[nodiscard]] double endBar() const;
    /// Whole bars up to the end of the track, for the length of the timeline.
    [[nodiscard]] int lengthBars() const;
    /// Second of the file that plays at the bar of the song (F-BT-04).
    [[nodiscard]] double fileSecondsAtBar(double bar) const;

    /// Offset in milliseconds: the time of the file that plays at the song start (F-BT-05). Repeated changes
    /// form one undo step.
    [[nodiscard]] double offsetMs() const;
    void setOffsetMs(double offsetMs);
    /// Dragging the waveform: deltaBars to the right moves the track later; one undo step per drag.
    void beginOffsetDrag();
    void dragOffsetBy(double deltaBars);
    void endOffsetDrag();

    /// Call from the GUI timer: frees streams the audio thread no longer uses.
    void tick();
    /// Changes whenever the project changed, also by undo or loading.
    [[nodiscard]] std::uint32_t changeCount() const noexcept { return changeCount_; }

private:
    [[nodiscard]] model::Project project() const;
    [[nodiscard]] double secondsPerBar() const;
    [[nodiscard]] double offsetSeconds() const;
    void projectChanged();
    void open(const std::filesystem::path& file);
    void setOffsetSamples(std::int64_t offset);

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
    IBackingTrackLoader& loader_;
    engine::BackingTrackPlayer& player_;
    std::filesystem::path currentFile_; ///< file last opened for the player, empty if none
    double sampleRate_ = 0.0;           ///< of the opened file, 0 if none
    std::int64_t lengthSamples_ = 0;
    std::optional<std::int64_t> dragStart_;
    std::uint32_t changeCount_ = 0;
    model::TreeChangeListener listener_;
};

} // namespace drumprog::ui
