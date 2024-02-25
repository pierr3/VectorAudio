#pragma once
#include "afv-native/atcClientWrapper.h"
#include "imgui.h"
#include "shared.h"
#include "ui/style.h"

#include <string>
#include <vector>

namespace vector_audio::ui::widgets {
class GainWidget {

public:
    static void Draw(bool isVoiceConnected, std::function<void()> callback)
    {
        ImGui::PushItemWidth(-1.0);
        ImGui::Text("Radio Gain");
        style::push_disabled_on(!isVoiceConnected);
        if (ImGui::SliderInt(
                "##Radio Gain", &shared::radioGain, 0, 200, "%.3i %%")) {
            if (isVoiceConnected) {
                std::invoke(callback);
            }
        }
        ImGui::PopItemWidth();
        style::pop_disabled_on(!isVoiceConnected);
    }
};
}
