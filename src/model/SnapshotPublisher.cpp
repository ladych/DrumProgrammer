#include "model/SnapshotPublisher.h"

#include "model/Project.h"
#include "model/SnapshotBuilder.h"

#include <utility>

namespace drumprog::model
{

SnapshotPublisher::SnapshotPublisher(juce::ValueTree project,
                                     ProjectSnapshotExchange& exchange,
                                     engine::IKitSink& kitSink)
    : project_(std::move(project)), exchange_(exchange), kitSink_(kitSink),
      listener_(project_, [this] { publish(); })
{
    publish();
}

void SnapshotPublisher::publish()
{
    const Project project{project_, nullptr};
    exchange_.publish(SnapshotBuilder::build(project));
    kitSink_.publishKit(SnapshotBuilder::buildKit(project.kit()));
}

} // namespace drumprog::model
