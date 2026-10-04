#include "model/SnapshotPublisher.h"

#include "model/Project.h"
#include "model/SnapshotBuilder.h"

#include <utility>

namespace drumprog::model
{

SnapshotPublisher::SnapshotPublisher(juce::ValueTree project,
                                     juce::ValueTree globalKit,
                                     ProjectSnapshotExchange& exchange,
                                     engine::IKitSink& kitSink)
    : project_(std::move(project)), globalKit_(std::move(globalKit)), exchange_(exchange), kitSink_(kitSink),
      projectListener_(project_, [this] { publish(); }), globalKitListener_(globalKit_, [this] { publish(); })
{
    publish();
}

void SnapshotPublisher::publish()
{
    const Project project{project_, nullptr};
    const auto kit = project.activeKit(globalKit_);
    exchange_.publish(SnapshotBuilder::build(project, kit));
    kitSink_.publishKit(SnapshotBuilder::buildKit(kit));
}

} // namespace drumprog::model
