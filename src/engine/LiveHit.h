#pragma once

namespace drumprog::engine
{

/// A hit of the live input as the audio thread played it, for the recording (F-IN-07).
struct LiveHit
{
    int slotIndex = 0;
    int velocity = 0;
    double timeSeconds = 0.0; ///< when the input thread received it, on the Clock of engine/Clock.h
};

} // namespace drumprog::engine
