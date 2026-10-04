#pragma once

#include "engine/KitDescription.h"

namespace drumprog::engine
{

/// Receives the kit whenever the project's kit changes, so the engine can rebuild what it plays.
/// GUI thread only.
class IKitSink
{
public:
    virtual ~IKitSink() = default;

    virtual void publishKit(const KitDescription& kit) = 0;

protected:
    IKitSink() = default;
    IKitSink(const IKitSink&) = default;
    IKitSink(IKitSink&&) = default;
    IKitSink& operator=(const IKitSink&) = default;
    IKitSink& operator=(IKitSink&&) = default;
};

} // namespace drumprog::engine
