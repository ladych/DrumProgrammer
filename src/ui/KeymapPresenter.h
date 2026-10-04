#pragma once

#include "input/KeyNames.h"
#include "input/Keymap.h"
#include "io/SettingsStore.h"
#include "ui/KitPresenter.h"

#include <optional>
#include <string>

namespace drumprog::ui
{

/// Logic of the dialog "Tastatur-Mapping" and the key field in the inspector (F-IN-02, humble views
/// app/KeymapDialog and app/KitPanel). Rows are the kit's slots; a slot's key is the key of its MIDI
/// note. Every change is saved as program setting right away. GUI thread only.
class KeymapPresenter
{
public:
    KeymapPresenter(input::Keymap& keymap,
                    io::SettingsStore& store,
                    const input::IKeyNames& keyNames,
                    const KitPresenter& kit);

    [[nodiscard]] int numRows() const;
    [[nodiscard]] std::string noteLabel(int row) const;
    [[nodiscard]] std::string slotName(int row) const;
    /// Name of the key, "–" without key, or the prompt while the row is learning.
    [[nodiscard]] std::string keyLabel(int row) const;
    /// True if the row's key also triggers another note.
    [[nodiscard]] bool isDuplicate(int row) const;
    /// One line per key given to several slots; empty without duplicates.
    [[nodiscard]] std::string duplicateWarning() const;

    /// Learning mode: the next key pressed becomes the row's key. Ignores rows outside the kit.
    void startLearning(int row);
    void cancelLearning();
    [[nodiscard]] std::optional<int> learningRow() const noexcept { return learning_; }
    /// While learning: assigns the key, or cancels on Escape, and returns true. Modifier keys keep
    /// the learning mode. Returns false when not learning, so the key may trigger a drum.
    bool captureKey(int scancode);

    void clearKey(int row);
    void restoreDefaults();

private:
    [[nodiscard]] bool isRow(int row) const;
    [[nodiscard]] std::string slotList(const std::vector<int>& notes) const;
    void save();

    input::Keymap& keymap_;
    io::SettingsStore& store_;
    const input::IKeyNames& keyNames_;
    const KitPresenter& kit_;
    std::optional<int> learning_;
};

} // namespace drumprog::ui
