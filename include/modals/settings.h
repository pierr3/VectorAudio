#pragma once
#include "afv-native/atcClientWrapper.h"
#include "config.h"
#include "data_file_handler.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "shared.h"
#include "style.h"
#include "util.h"

#include <imgui_stdlib.h>
#include <SFML/System/String.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <string>

namespace vector_audio::modals {
class Settings {
public:
    static void render(afv_native::api::atcClient* mClient,
        const std::function<void()>& playAlertSound);
};
}