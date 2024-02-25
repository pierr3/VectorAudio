#include "native/window_manager.h"


namespace vector_audio {

void setAlwaysOnTop(SDL_Window* window, bool alwaysOnTop) {
    SDL_SetWindowAlwaysOnTop(window, alwaysOnTop ? SDL_TRUE : SDL_FALSE);
}

}