#pragma once

#include "engine/IKitSink.h"
#include "engine/ProjectSnapshot.h"
#include "engine/SnapshotExchange.h"
#include "model/TreeChangeListener.h"

#include <juce_data_structures/juce_data_structures.h>

namespace drumprog::model
{

using ProjectSnapshotExchange = engine::SnapshotExchange<engine::ProjectSnapshot>;

/// The single connection from the project model to the engine: on construction and after every change
/// of the project tree it publishes a new snapshot for the audio thread and hands the kit to the kit
/// sink, so loading, editing or undoing a project changes what is heard. Runs on the GUI thread.
class SnapshotPublisher
{
public:
    SnapshotPublisher(juce::ValueTree project, ProjectSnapshotExchange& exchange, engine::IKitSink& kitSink);

    void publish();

private:
    juce::ValueTree project_;
    ProjectSnapshotExchange& exchange_;
    engine::IKitSink& kitSink_;
    TreeChangeListener listener_;
};

} // namespace drumprog::model
