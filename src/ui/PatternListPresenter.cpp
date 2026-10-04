#include "ui/PatternListPresenter.h"

#include <algorithm>
#include <array>
#include <utility>

namespace drumprog::ui
{
namespace
{

constexpr const char* kCopySuffix = " Kopie";
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
    return std::to_string(bars) + (bars == 1 ? " Takt" : " Takte");
}

void PatternListPresenter::add()
{
    undoManager_.beginNewTransaction("Pattern anlegen");
    auto pattern = project().addPattern(idGenerator_.next(), freeName(), kNewPatternBars);
    pattern.setColour(kPalette.at(static_cast<std::size_t>(numPatterns() - 1) % kPalette.size()));
    active_.select(numPatterns() - 1);
}

void PatternListPresenter::rename(int index, const std::string& name)
{
    const auto trimmed = juce::String::fromUTF8(name.c_str()).trim().toStdString();
    if (!isValid(index) || trimmed.empty())
        return;
    undoManager_.beginNewTransaction("Pattern umbenennen");
    project().pattern(index).setName(trimmed);
}

void PatternListPresenter::setColour(int index, const std::string& colour)
{
    if (!isValid(index))
        return;
    undoManager_.beginNewTransaction("Pattern-Farbe");
    project().pattern(index).setColour(colour);
}

void PatternListPresenter::setLengthBars(int index, int bars)
{
    if (!isValid(index))
        return;
    undoManager_.beginNewTransaction(juce::String::fromUTF8("Pattern-L\xc3\xa4nge"));
    project().pattern(index).setLengthBars(std::clamp(bars, kMinBars, kMaxBars));
}

void PatternListPresenter::duplicate(int index)
{
    if (!isValid(index))
        return;
    undoManager_.beginNewTransaction("Pattern duplizieren");
    project().duplicatePattern(index, idGenerator_.next(), name(index) + kCopySuffix);
    active_.select(index + 1);
}

bool PatternListPresenter::canRemove() const
{
    return numPatterns() > 1;
}

void PatternListPresenter::remove(int index)
{
    if (!isValid(index) || !canRemove())
        return;
    undoManager_.beginNewTransaction(juce::String::fromUTF8("Pattern l\xc3\xb6schen"));
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
    while (nameExists("Pattern " + std::to_string(number)))
        ++number;
    return "Pattern " + std::to_string(number);
}

} // namespace drumprog::ui
