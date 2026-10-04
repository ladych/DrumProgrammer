#pragma once

#include "engine/SampleBuffer.h"

namespace drumprog::engine
{

/// Mixes multi-microphone recordings (e.g. 16-channel DrumGizmo kits) down to
/// stereo when they are loaded, so the voices only ever play mono or stereo.
/// Without a channel layout every channel goes to both sides with the same
/// gain of 1/sqrt(channels). Mono and stereo samples are returned unchanged.
[[nodiscard]] SampleBuffer downmixToStereo(const SampleBuffer& source);

} // namespace drumprog::engine
