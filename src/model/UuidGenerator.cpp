#include "model/UuidGenerator.h"

#include <juce_core/juce_core.h>

namespace drumprog::model
{

std::string UuidGenerator::next()
{
    return juce::Uuid{}.toDashedString().toStdString();
}

} // namespace drumprog::model
