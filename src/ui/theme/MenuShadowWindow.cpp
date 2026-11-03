#include "ui/theme/MenuShadowWindow.h"

namespace calliope
{
using namespace juce;

namespace
{
// The same shadow as LookAndFeel_V2::createDropShadowerForComponent.
const DropShadow shadow{Colours::black.withAlpha(0.4f), 10, {0, 2}};
const int shadowEdge = jmax(shadow.offset.x, shadow.offset.y) + shadow.radius;
} // namespace

MenuShadowWindow::MenuShadowWindow()
{
    setInterceptsMouseClicks(false, false);
    setAccessible(false);
    setSize(1, 1);
    addToDesktop(ComponentPeer::windowIgnoresMouseClicks | ComponentPeer::windowIsTemporary |
                 ComponentPeer::windowIgnoresKeyPresses);
}

MenuShadowWindow::~MenuShadowWindow()
{
    detach();
}

bool MenuShadowWindow::isAttached() const
{
    return target != nullptr;
}

bool MenuShadowWindow::isAttachedTo(const Component& component) const
{
    return target.get() == &component;
}

void MenuShadowWindow::attachTo(Component& component)
{
    if (isAttachedTo(component))
        return;

    detach();
    target = &component;
    component.addComponentListener(this);
    followTarget();
}

void MenuShadowWindow::paint(Graphics& g)
{
    auto* c = target.get();
    if (c == nullptr)
        return;

    // A menu owned by another window can drop below this one when a neighbouring menu closes, so the
    // target's area is left clear instead of relying on the stacking order.
    const auto area = getLocalArea(c, c->getLocalBounds());
    g.excludeClipRegion(area);
    shadow.drawForRectangle(g, area);
}

float MenuShadowWindow::getDesktopScaleFactor() const
{
    if (auto* c = target.get())
        return c->getDesktopScaleFactor();

    return Component::getDesktopScaleFactor();
}

void MenuShadowWindow::detach()
{
    if (auto* c = target.get())
        c->removeComponentListener(this);

    target = nullptr;
    setVisible(false);
}

void MenuShadowWindow::followTarget()
{
    if (isFollowing)
        return;

    const ScopedValueSetter<bool> setter(isFollowing, true);

    auto* c = target.get();
    if (c == nullptr || !c->isShowing() || c->getWidth() <= 0 || c->getHeight() <= 0)
    {
        setVisible(false);
        return;
    }

    setAlwaysOnTop(c->isAlwaysOnTop());
    setBounds(c->getBounds().expanded(shadowEdge));
    setVisible(true);
    toBehind(c);
    repaint();
}

void MenuShadowWindow::componentMovedOrResized(Component&, bool, bool)
{
    followTarget();
}

void MenuShadowWindow::componentVisibilityChanged(Component&)
{
    followTarget();
}

void MenuShadowWindow::componentBroughtToFront(Component&)
{
    followTarget();
}

void MenuShadowWindow::componentBeingDeleted(Component&)
{
    detach();
}
} // namespace calliope
