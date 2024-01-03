#include "native/window_manager.h"

#include <iostream>

#ifdef SFML_SYSTEM_WINDOWS
#include <Windows.h>
#elif defined(SFML_SYSTEM_LINUX)
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#endif

namespace vector_audio {

#if !defined(SFML_SYSTEM_MACOS)

void setAlwaysOnTop(sf::RenderWindow& window, bool alwaysOnTop) {
#ifdef SFML_SYSTEM_WINDOWS

    HWND hwnd = window.getSystemHandle();
    SetWindowPos(hwnd, alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    
#elif defined(SFML_SYSTEM_LINUX)

    XEvent event;
    Atom wmStateAbove;
    Atom wmNetWmState;

    auto display = XOpenDisplay(NULL);
    if(!display) {
        std::cerr << "WindowManager: failed to open display" << std::endl;
        return;
    }

    auto xwin = static_cast<Window>(window.getSystemHandle());
    if(!xwin) {
        std::cerr << "WindowManager: getSystemHandle is null" << std::endl;
        return;
    }

    wmStateAbove = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    wmNetWmState = XInternAtom(display, "_NET_WM_STATE", False);

    event.xclient.type = ClientMessage;
    event.xclient.display = display;
    event.xclient.window = xwin;
    event.xclient.message_type = wmNetWmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = alwaysOnTop ? 1 : 0; // _NET_WM_STATE_ADD
    event.xclient.data.l[1] = wmStateAbove;
    event.xclient.data.l[2] = 0; // No second property
    event.xclient.data.l[3] = 1; // Normal application window

    XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XSync(display, False);
    XCloseDisplay(display);

#endif
}

#endif

}