#include "native/window_manager.h"

#import <Cocoa/Cocoa.h>

namespace vector_audio {

    void setAlwaysOnTop(sf::RenderWindow& window, bool alwaysOnTop) {
        NSWindow* nsWindow = static_cast<NSWindow*>(window.getSystemHandle());
        if (alwaysOnTop) {
            [nsWindow setLevel:NSFloatingWindowLevel];
        } else {
            [nsWindow setLevel:NSNormalWindowLevel];
        }
    }

}