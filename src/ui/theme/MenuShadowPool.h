#pragma once

#include "ui/theme/MenuShadowWindow.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

namespace calliope
{
class MenuShadowPool
{
public:
    void prepare();
    void attachTo(juce::Component& menu);

private:
    std::vector<std::unique_ptr<MenuShadowWindow>> windows;
};
} // namespace calliope
