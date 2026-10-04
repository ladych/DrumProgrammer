#pragma once

#include "engine/SampleBuffer.h"

namespace drumprog::engine
{

/// Converts a sample to the device sample rate when it is loaded (F-SE-02), so
/// the audio thread only has to play it back. Uses 4-point cubic Hermite
/// interpolation, which is good enough for one-shot drum samples at the usual
/// rates (44.1/48/88.2/96 kHz).
[[nodiscard]] SampleBuffer resample(const SampleBuffer& source, double targetSampleRate);

} // namespace drumprog::engine
