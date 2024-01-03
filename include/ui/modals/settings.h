#pragma once
#include "afv-native/atcClientWrapper.h"
#include "config.h"
#include "data_file_handler.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "shared.h"
#include "ui/style.h"
#include "util.h"

#include <imgui_stdlib.h>
#include <SFML/System/String.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <string>

namespace vector_audio::ui::modals {
class Settings {
public:
    static void render(const std::shared_ptr<afv_native::api::atcClient>& mClient,
        const std::function<void()>& playAlertSound);
};
}