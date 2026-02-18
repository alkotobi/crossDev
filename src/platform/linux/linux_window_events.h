#ifndef LINUX_WINDOW_EVENTS_H
#define LINUX_WINDOW_EVENTS_H

#ifdef PLATFORM_LINUX
#include <X11/Xlib.h>

namespace platform {
// Dispatch focus/blur events to registered window callbacks.
// Call this from the main event loop for each XEvent.
void dispatchFocusEvent(Display* dpy, XEvent* ev);

// Register app-level activate/deactivate callbacks (called from setAppActivate/DeactivateCallback).
void registerAppFocusCallbacks(void (*activate)(void*), void* activateUd,
                               void (*deactivate)(void*), void* deactivateUd);

// Dispatch ConfigureNotify (resize/move) to registered window callbacks.
void dispatchConfigureEvent(Display* dpy, XEvent* ev);

// Dispatch UnmapNotify/MapNotify/PropertyNotify for window state (minimize/maximize/restore).
void dispatchWindowStateEvent(Display* dpy, XEvent* ev);
}  // namespace platform

#endif  // PLATFORM_LINUX

#endif  // LINUX_WINDOW_EVENTS_H
