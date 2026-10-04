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
/// of the project tree or the global kit it publishes a new snapshot for the audio thread and hands the
/// active kit (the project's own kit, otherwise the global kit) to the kit sink, so loading, editing or
/// undoing changes what is heard. Runs on the GUI thread.
class SnapshotPublisher
{
public:
    SnapshotPublisher(juce::ValueTree project,
                      juce::ValueTree globalKit,
                      ProjectSnapshotExchange& exchange,
                      engine::IKitSink& kitSink);

    void publish();

private:
    juce::ValueTree project_;
    juce::ValueTree globalKit_;
    ProjectSnapshotExchange& exchange_;
    engine::IKitSink& kitSink_;
    TreeChangeListener projectListener_;
    TreeChangeListener globalKitListener_;
};

} // namespace drumprog::model
