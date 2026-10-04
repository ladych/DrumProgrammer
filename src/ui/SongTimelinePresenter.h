#pragma once

#include "model/Project.h"
#include "model/TreeChangeListener.h"
#include "ui/ActivePattern.h"

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace drumprog::ui
{

/// A block on the drums track, in bars from the song start (bar 0 is shown as 1).
struct SongBlockView
{
    int patternIndex = 0;
    int startBar = 0;
    int lengthBars = 0;
    std::string name;
    std::string colour; ///< "#RRGGBB", the pattern's colour
    bool selected = false;
};

/// Where a dragged block or a dropped pattern would land.
struct SongGhost
{
    int patternIndex = 0;
    int startBar = 0;
    int lengthBars = 0;
    bool duplicate = false; ///< Alt-drag copies the block
};

/// Logic of the song timeline (F-SO-02 to 07, humble view app/SongTimelineView) and the menu "Song".
/// Blocks are addressed by their index in blocks(), which is sorted by start; later blocks lie on top
/// of earlier ones and cut them off where they start (model::layoutSong). Blocks reference their
/// pattern, so changing a pattern changes all its blocks (F-SO-04). Every edit is one undo step
/// (F-PJ-05). GUI thread only.
class SongTimelinePresenter
{
public:
    SongTimelinePresenter(juce::ValueTree project, juce::UndoManager& undoManager, ActivePattern& active);

    [[nodiscard]] std::vector<SongBlockView> blocks() const;
    /// End of the last block in bars.
    [[nodiscard]] int songLengthBars() const;
    [[nodiscard]] std::int64_t ticksPerBar() const;
    [[nodiscard]] bool isEmpty() const;
    /// Topmost block covering the bar, -1 if none.
    [[nodiscard]] int blockAt(int bar) const;

    /// -1 or an index outside the blocks clears the selection.
    void select(int block);
    [[nodiscard]] int selectedBlock() const;

    /// Places the pattern at the bar (drag & drop from the pattern list, F-SO-02) and selects it.
    void insertPattern(int patternIndex, int bar);
    /// Menu "Pattern einfügen": the active pattern behind the last block.
    void appendActivePattern();
    /// Menu "Block löschen".
    void removeSelected();
    /// Menu "Song leeren".
    void clear();
    /// Double click: opens the block's pattern in the piano roll (F-SO-06).
    void open(int block);

    // Moving blocks bar by bar, Alt duplicates (F-SO-03). The preview is ghost(); release() commits.
    void press(int block, int bar);
    void dragTo(int bar, bool duplicate);
    void release();

    // A pattern dragged from the list over the track.
    void hoverPattern(int patternIndex, int bar);
    void endHover();

    [[nodiscard]] std::optional<SongGhost> ghost() const;

    /// Changes whenever the project changed, also by undo or loading.
    [[nodiscard]] std::uint32_t changeCount() const noexcept { return changeCount_; }

private:
    struct Drag
    {
        juce::ValueTree entry;
        int patternIndex = 0;
        int lengthBars = 0;
        int originBar = 0;
        int grabOffset = 0;
        int targetBar = 0;
        bool duplicate = false;
    };

    [[nodiscard]] model::Project project() const;
    [[nodiscard]] juce::ValueTree entryOf(int block) const;
    [[nodiscard]] bool isValidPattern(int patternIndex) const;
    void add(int patternIndex, int bar);

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
    ActivePattern& active_;
    juce::ValueTree selected_;
    std::optional<Drag> drag_;
    std::optional<SongGhost> hover_;
    std::uint32_t changeCount_ = 0;
    model::TreeChangeListener listener_;
};

} // namespace drumprog::ui
