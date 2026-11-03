#include "ui/theme/MenuShadowPool.h"
#include <algorithm>
#include <iterator>

namespace calliope
{
void MenuShadowPool::prepare()
{
    if (windows.empty())
        windows.push_back(std::make_unique<MenuShadowWindow>());
}

void MenuShadowPool::attachTo(juce::Component& menu)
{
    // JUCE asks for a shadow more than once for the same menu.
    if (std::ranges::any_of(windows, [&](const auto& w) { return w->isAttachedTo(menu); }))
        return;

    auto free = std::ranges::find_if(windows, [](const auto& w) { return !w->isAttached(); });
    if (free == windows.end())
    {
        windows.push_back(std::make_unique<MenuShadowWindow>());
        free = std::prev(windows.end());
    }

    (*free)->attachTo(menu);
}
} // namespace calliope
