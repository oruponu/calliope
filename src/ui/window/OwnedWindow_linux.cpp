#include "ui/window/OwnedWindow.h"

// After the JUCE headers: Xlib defines macros such as None and Bool.
#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace
{
::Window toXWindow(void* nativeHandle)
{
    // JUCE's native handle on Linux is the X window id stored in a pointer.
    return static_cast<::Window>(reinterpret_cast<juce::pointer_sized_uint>(nativeHandle));
}
} // namespace

void keepAboveOwner(juce::Component& window, juce::Component& owner)
{
    auto* windowPeer = window.getPeer();
    auto* ownerPeer = owner.getTopLevelComponent()->getPeer();
    if (windowPeer == nullptr || ownerPeer == nullptr)
        return;

    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr)
        return;

    XSetTransientForHint(display, toXWindow(windowPeer->getNativeHandle()), toXWindow(ownerPeer->getNativeHandle()));
    XFlush(display);
    XCloseDisplay(display);
}

// A hidden transient window stays hidden when its owner comes to the front.
void releaseFromOwner(juce::Component&) {}
