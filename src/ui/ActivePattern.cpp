#include "ui/ActivePattern.h"

#include "model/Project.h"

#include <algorithm>
#include <utility>

namespace drumprog::ui
{

ActivePattern::ActivePattern(juce::ValueTree project)
    : project_(std::move(project)), listener_(project_, [this] { update(); })
{
    update();
}

void ActivePattern::select(int index)
{
    if (index >= 0 && index < numPatterns())
        setIndex(index);
}

void ActivePattern::update()
{
    const int found = indexOf(id_);
    setIndex(found >= 0 ? found : std::min(index_, numPatterns() - 1));
}

int ActivePattern::numPatterns() const
{
    return model::Project{project_, nullptr}.numPatterns();
}

int ActivePattern::indexOf(const std::string& id) const
{
    const model::Project project{project_, nullptr};
    for (int index = 0; index < project.numPatterns(); ++index)
        if (project.pattern(index).id() == id)
            return index;
    return -1;
}

void ActivePattern::setIndex(int index)
{
    index = std::max(index, numPatterns() > 0 ? 0 : -1);
    id_.clear();
    if (index >= 0)
        id_ = model::Project{project_, nullptr}.pattern(index).id();
    if (index == index_)
        return;
    index_ = index;
    for (const auto& onChange : onChange_)
        onChange(index_);
}

} // namespace drumprog::ui
