#include "ui/TempoPresenter.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace drumprog::ui
{
namespace
{

constexpr double kHundredths = 100.0;

} // namespace

TempoPresenter::TempoPresenter(juce::ValueTree project, juce::UndoManager& undoManager)
    : project_(std::move(project)), undoManager_(undoManager)
{
}

double TempoPresenter::bpm() const
{
    return project().bpm();
}

std::string TempoPresenter::bpmText() const
{
    std::ostringstream text;
    text << std::fixed << std::setprecision(2) << bpm();
    return text.str();
}

void TempoPresenter::setBpm(double bpm)
{
    const double clamped = std::clamp(bpm, kMinBpm, kMaxBpm);
    undoManager_.beginNewTransaction("Tempo");
    project().setBpm(std::round(clamped * kHundredths) / kHundredths);
}

bool TempoPresenter::setBpmText(const std::string& text)
{
    std::string number = text;
    std::ranges::replace(number, ',', '.');
    std::erase(number, ' ');
    double value = 0.0;
    const auto* const end = number.data() + number.size();
    const auto [parsedUntil, error] = std::from_chars(number.data(), end, value);
    if (error != std::errc{} || parsedUntil != end)
        return false;
    setBpm(value);
    return true;
}

model::TimeSignature TempoPresenter::timeSignature() const
{
    return project().timeSignature();
}

std::string TempoPresenter::timeSignatureText() const
{
    const auto signature = timeSignature();
    return std::to_string(signature.numerator) + "/" + std::to_string(signature.denominator);
}

void TempoPresenter::setTimeSignature(int numerator, int denominator)
{
    if (!isValidDenominator(denominator))
        return;
    undoManager_.beginNewTransaction("Taktart");
    project().setTimeSignature({std::clamp(numerator, 1, kMaxNumerator), denominator});
}

bool TempoPresenter::isValidDenominator(int denominator) noexcept
{
    return denominator == 4 || denominator == 8 || denominator == 16;
}

model::Project TempoPresenter::project() const
{
    return {project_, &undoManager_};
}

} // namespace drumprog::ui
