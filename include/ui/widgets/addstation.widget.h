#pragma once
#include "afv-native/atcClientWrapper.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "shared.h"
#include "ui/style.h"

#include <functional>
#include <string>
#include <vector>

namespace vector_audio::ui::widgets {
class AddStationWidget {

public:
    static void Draw(const std::shared_ptr<afv_native::api::atcClient>& pClient,
        const std::function<void()>& addCallback)
    {
        ImGui::PushItemWidth(-1.0);
        ImGui::Text("Add station");

        style::push_disabled_on(!pClient->IsVoiceConnected());
        if (ImGui::InputText("Callsign##Auto", &shared::stationAutoAddCallsign,
                ImGuiInputTextFlags_EnterReturnsTrue
                    | ImGuiInputTextFlags_AutoSelectAll
                    | ImGuiInputTextFlags_CharsUppercase)
            || ImGui::Button("Add", ImVec2(-FLT_MIN, 0.0))) {
            if (pClient->IsVoiceConnected()) {
                addCallback();
            }
        }
        ImGui::PopItemWidth();
        style::pop_disabled_on(!pClient->IsVoiceConnected());
    }
};
}
