#include "ui/BackingTrackPresenter.h"

#include "io/Utf8Path.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace drumprog::ui
{
namespace
{

constexpr double kMillisecondsPerSecond = 1000.0;
constexpr double kSecondsPerMinute = 60.0;
constexpr const char* kOffsetUndoName = "Versatz Backing-Track";

} // namespace

BackingTrackPresenter::BackingTrackPresenter(juce::ValueTree project,
                                             juce::UndoManager& undoManager,
                                             IBackingTrackLoader& loader,
                                             engine::BackingTrackPlayer& player)
    : project_(std::move(project)), undoManager_(undoManager), loader_(loader), player_(player),
      listener_(project_, [this] { projectChanged(); })
{
    projectChanged();
}

bool BackingTrackPresenter::load(const std::filesystem::path& file)
{
    auto stream = loader_.open(file);
    if (stream == nullptr)
        return false;
    currentFile_ = file;
    sampleRate_ = stream->sampleRate();
    lengthSamples_ = stream->lengthInSamples();
    player_.setStream(std::move(stream));
    undoManager_.beginNewTransaction("Backing-Track laden");
    auto track = project().backingTrack();
    track.setFilePath(io::utf8FromPath(file));
    track.setOffsetSamples(0);
    undoManager_.beginNewTransaction();
    return true;
}

void BackingTrackPresenter::remove()
{
    if (!hasTrack())
        return;
    undoManager_.beginNewTransaction("Backing-Track entfernen");
    auto track = project().backingTrack();
    track.setFilePath({});
    track.setOffsetSamples(0);
    undoManager_.beginNewTransaction();
}

bool BackingTrackPresenter::hasTrack() const
{
    return !project().backingTrack().filePath().empty();
}

bool BackingTrackPresenter::isMissing() const
{
    return hasTrack() && sampleRate_ <= 0.0;
}

std::filesystem::path BackingTrackPresenter::file() const
{
    return io::pathFromUtf8(project().backingTrack().filePath());
}

std::string BackingTrackPresenter::label() const
{
    if (!hasTrack())
        return {};
    auto name = io::utf8FromPath(file().filename());
    if (isMissing())
        return name + " (Datei fehlt)";
    return name;
}

double BackingTrackPresenter::lengthSeconds() const
{
    if (sampleRate_ <= 0.0)
        return 0.0;
    return static_cast<double>(lengthSamples_) / sampleRate_;
}

double BackingTrackPresenter::startBar() const
{
    if (sampleRate_ <= 0.0)
        return 0.0;
    return -offsetSeconds() / secondsPerBar();
}

double BackingTrackPresenter::endBar() const
{
    if (sampleRate_ <= 0.0)
        return 0.0;
    return (lengthSeconds() - offsetSeconds()) / secondsPerBar();
}

int BackingTrackPresenter::lengthBars() const
{
    return static_cast<int>(std::ceil(std::max(endBar(), 0.0)));
}

double BackingTrackPresenter::fileSecondsAtBar(double bar) const
{
    return bar * secondsPerBar() + offsetSeconds();
}

double BackingTrackPresenter::offsetMs() const
{
    return offsetSeconds() * kMillisecondsPerSecond;
}

void BackingTrackPresenter::setOffsetMs(double offsetMs)
{
    if (sampleRate_ <= 0.0)
        return;
    if (undoManager_.getCurrentTransactionName() != kOffsetUndoName)
        undoManager_.beginNewTransaction(kOffsetUndoName);
    setOffsetSamples(std::llround(offsetMs / kMillisecondsPerSecond * sampleRate_));
}

void BackingTrackPresenter::beginOffsetDrag()
{
    if (sampleRate_ <= 0.0)
        return;
    dragStart_ = project().backingTrack().offsetSamples();
    undoManager_.beginNewTransaction("Backing-Track verschieben");
}

void BackingTrackPresenter::dragOffsetBy(double deltaBars)
{
    if (!dragStart_)
        return;
    setOffsetSamples(*dragStart_ - std::llround(deltaBars * secondsPerBar() * sampleRate_));
}

void BackingTrackPresenter::endOffsetDrag()
{
    if (!dragStart_)
        return;
    dragStart_.reset();
    undoManager_.beginNewTransaction();
}

void BackingTrackPresenter::tick()
{
    player_.collectGarbage();
}

model::Project BackingTrackPresenter::project() const
{
    return {project_, &undoManager_};
}

double BackingTrackPresenter::secondsPerBar() const
{
    const auto project = this->project();
    return static_cast<double>(project.ticksPerBar()) / project.ticksPerQuarter() * kSecondsPerMinute /
           project.bpm();
}

double BackingTrackPresenter::offsetSeconds() const
{
    if (sampleRate_ <= 0.0)
        return 0.0;
    return static_cast<double>(project().backingTrack().offsetSamples()) / sampleRate_;
}

void BackingTrackPresenter::projectChanged()
{
    ++changeCount_;
    const auto path = file();
    if (path != currentFile_)
        open(path);
}

void BackingTrackPresenter::open(const std::filesystem::path& file)
{
    currentFile_ = file;
    sampleRate_ = 0.0;
    lengthSamples_ = 0;
    auto stream = file.empty() ? nullptr : loader_.open(file);
    if (stream != nullptr)
    {
        sampleRate_ = stream->sampleRate();
        lengthSamples_ = stream->lengthInSamples();
    }
    player_.setStream(std::move(stream));
}

void BackingTrackPresenter::setOffsetSamples(std::int64_t offset)
{
    project().backingTrack().setOffsetSamples(offset);
}

} // namespace drumprog::ui
