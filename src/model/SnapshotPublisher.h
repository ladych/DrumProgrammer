#pragma once

#include "engine/ProjectSnapshot.h"
#include "engine/SnapshotExchange.h"
#include "model/TreeChangeListener.h"

#include <juce_data_structures/juce_data_structures.h>

namespace drumprog::model
{

using ProjectSnapshotExchange = engine::SnapshotExchange<engine::ProjectSnapshot>;

/// Keeps the audio thread's snapshot up to date: publishes a new one on construction and after
/// every change of the project tree. Runs on the GUI thread.
class SnapshotPublisher
{
public:
    SnapshotPublisher(juce::ValueTree project, ProjectSnapshotExchange& exchange);

    void publish();

private:
    juce::ValueTree project_;
    ProjectSnapshotExchange& exchange_;
    TreeChangeListener listener_;
};

} // namespace drumprog::model
