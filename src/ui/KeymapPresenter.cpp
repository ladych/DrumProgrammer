#include "ui/KeymapPresenter.h"

#include "input/Scancode.h"

namespace drumprog::ui
{
namespace
{

constexpr const char* kNoKey = "–";
constexpr const char* kLearningPrompt = "Taste drücken …";

} // namespace

KeymapPresenter::KeymapPresenter(input::Keymap& keymap,
                                 io::SettingsStore& store,
                                 const input::IKeyNames& keyNames,
                                 const KitPresenter& kit)
    : keymap_(keymap), store_(store), keyNames_(keyNames), kit_(kit)
{
}

int KeymapPresenter::numRows() const
{
    return kit_.numSlots();
}

std::string KeymapPresenter::noteLabel(int row) const
{
    return isRow(row) ? std::to_string(kit_.midiNote(row)) : std::string{};
}

std::string KeymapPresenter::slotName(int row) const
{
    return kit_.slotName(row);
}

std::string KeymapPresenter::keyLabel(int row) const
{
    if (!isRow(row))
        return {};
    if (learning_ == row)
        return kLearningPrompt;
    const auto key = keymap_.keyFor(kit_.midiNote(row));
    if (!key)
        return kNoKey;
    return keyNames_.nameOf(*key);
}

bool KeymapPresenter::isDuplicate(int row) const
{
    if (!isRow(row))
        return false;
    const auto key = keymap_.keyFor(kit_.midiNote(row));
    if (!key)
        return false;
    return keymap_.notesFor(*key).size() > 1;
}

std::string KeymapPresenter::duplicateWarning() const
{
    std::string warning;
    for (const auto& [key, notes] : keymap_.duplicates())
        warning += "Taste " + keyNames_.nameOf(key) + " ist doppelt belegt: " + slotList(notes) + "\n";
    return warning;
}

void KeymapPresenter::startLearning(int row)
{
    if (isRow(row))
        learning_ = row;
}

void KeymapPresenter::cancelLearning()
{
    learning_.reset();
}

bool KeymapPresenter::captureKey(int scancode)
{
    if (!learning_)
        return false;
    if (input::isModifierKey(scancode))
        return true;
    if (scancode != input::scancode::kEscape)
    {
        keymap_.assign(kit_.midiNote(*learning_), scancode);
        save();
    }
    learning_.reset();
    return true;
}

void KeymapPresenter::clearKey(int row)
{
    if (!isRow(row))
        return;
    keymap_.clear(kit_.midiNote(row));
    save();
}

void KeymapPresenter::restoreDefaults()
{
    keymap_ = input::Keymap::defaults();
    learning_.reset();
    save();
}

bool KeymapPresenter::isRow(int row) const
{
    return row >= 0 && row < numRows();
}

std::string KeymapPresenter::slotList(const std::vector<int>& notes) const
{
    std::string list;
    for (const int note : notes)
    {
        if (!list.empty())
            list += ", ";
        list += std::to_string(note);
        for (int row = 0; row < numRows(); ++row)
            if (kit_.midiNote(row) == note)
                list += " " + kit_.slotName(row);
    }
    return list;
}

void KeymapPresenter::save()
{
    store_.save(keymap_.toText());
}

} // namespace drumprog::ui
