#include "ui/PatternListPresenter.h"

#include "i18n/Translate.h"

#include <algorithm>
#include <array>
#include <utility>

namespace drumprog::ui
{
namespace
{

constexpr std::array<const char*, 8> kPalette{
    "#E8743B", "#3B82F6", "#22C55E", "#EC4899", "#A855F7", "#EAB308", "#06B6D4", "#EF4444"};

} // namespace

PatternListPresenter::PatternListPresenter(juce::ValueTree project,
                                           juce::UndoManager& undoManager,
                                           model::IIdGenerator& idGenerator,
                                           ActivePattern& active)
    : project_(std::move(project)), undoManager_(undoManager), idGenerator_(idGenerator), active_(active),
      listener_(project_, [this] { ++changeCount_; })
{
}

std::vector<std::string> PatternListPresenter::palette()
{
    return {kPalette.begin(), kPalette.end()};
}

int PatternListPresenter::numPatterns() const
{
    return project().numPatterns();
}

std::string PatternListPresenter::name(int index) const
{
    if (!isValid(index))
        return {};
    return project().pattern(index).name();
}

std::string PatternListPresenter::colour(int index) const
{
    if (!isValid(index))
        return {};
    return project().pattern(index).colour();
}

int PatternListPresenter::lengthBars(int index) const
{
    if (!isValid(index))
        return {};
    return project().pattern(index).lengthBars();
}

std::string PatternListPresenter::lengthText(int index) const
{
    if (!isValid(index))
        return {};
    const int bars = lengthBars(index);
    if (bars == 1)
        return i18n::tr("%1 bar", std::to_string(bars));
    return i18n::tr("%1 bars", std::to_string(bars));
}

std::optional<int> PatternListPresenter::parseBars(const std::string& text)
{
    const auto trimmed = juce::String::fromUTF8(text.c_str()).trim();
    if (trimmed.isEmpty() || !trimmed.containsOnly("0123456789") || trimmed.length() > 2)
        return std::nullopt;
    const int bars = trimmed.getIntValue();
    if (bars < kMinBars || bars > kMaxBars)
        return std::nullopt;
    return bars;
}

void PatternListPresenter::add()
{
    add(freeName(), kNewPatternBars);
}

void PatternListPresenter::add(const std::string& name, int bars)
{
    auto trimmed = juce::String::fromUTF8(name.c_str()).trim().toStdString();
    if (trimmed.empty())
        trimmed = freeName();
    undoManager_.beginNewTransaction(i18n::trJuce("Add pattern"));
    auto pattern = project().addPattern(idGenerator_.next(), trimmed, std::clamp(bars, kMinBars, kMaxBars));
    pattern.setColour(kPalette.at(static_cast<std::size_t>(numPatterns() - 1) % kPalette.size()));
    active_.select(numPatterns() - 1);
}

void PatternListPresenter::rename(int index, const std::string& name)
{
    const auto trimmed = juce::String::fromUTF8(name.c_str()).trim().toStdString();
    if (!isValid(index) || trimmed.empty())
        return;
    undoManager_.beginNewTransaction(i18n::trJuce("Rename pattern"));
    project().pattern(index).setName(trimmed);
}

void PatternListPresenter::setColour(int index, const std::string& colour)
{
    if (!isValid(index))
        return;
    undoManager_.beginNewTransaction(i18n::trJuce("Pattern colour"));
    project().pattern(index).setColour(colour);
}

void PatternListPresenter::setLengthBars(int index, int bars)
{
    if (!isValid(index))
        return;
    undoManager_.beginNewTransaction(i18n::trJuce("Pattern length"));
    project().pattern(index).setLengthBars(std::clamp(bars, kMinBars, kMaxBars));
}

void PatternListPresenter::duplicate(int index)
{
    if (!isValid(index))
        return;
    undoManager_.beginNewTransaction(i18n::trJuce("Duplicate pattern"));
    project().duplicatePattern(index, idGenerator_.next(), i18n::tr("%1 copy", name(index)));
    active_.select(index + 1);
}

bool PatternListPresenter::canRemove() const
{
    return numPatterns() > 1;
}

int PatternListPresenter::songUses(int index) const
{
    if (!isValid(index))
        return 0;
    const auto project = this->project();
    const auto id = project.pattern(index).id();
    const auto song = project.song();
    int uses = 0;
    for (int entry = 0; entry < song.numEntries(); ++entry)
        if (song.entry(entry).patternId() == id)
            ++uses;
    return uses;
}

std::string PatternListPresenter::removeQuestion(int index) const
{
    const int uses = songUses(index);
    if (uses == 0)
        return {};
    return i18n::tr("\"%1\" is used %2 times in the song. Deleting it removes all its blocks.",
                    name(index),
                    std::to_string(uses));
}

void PatternListPresenter::remove(int index)
{
    if (!isValid(index) || !canRemove())
        return;
    undoManager_.beginNewTransaction(i18n::trJuce("Delete pattern"));
    project().removePattern(index);
}

model::Project PatternListPresenter::project() const
{
    return {project_, &undoManager_};
}

bool PatternListPresenter::isValid(int index) const
{
    return index >= 0 && index < numPatterns();
}

bool PatternListPresenter::nameExists(const std::string& name) const
{
    const auto project = this->project();
    for (int index = 0; index < project.numPatterns(); ++index)
        if (project.pattern(index).name() == name)
            return true;
    return false;
}

std::string PatternListPresenter::freeName() const
{
    int number = numPatterns() + 1;
    while (nameExists(i18n::tr("Pattern %1", std::to_string(number))))
        ++number;
    return i18n::tr("Pattern %1", std::to_string(number));
}

} // namespace drumprog::ui
