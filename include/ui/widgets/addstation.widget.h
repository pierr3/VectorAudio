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

protected:
    inline static std::string mStationCallsignInputString;

    const static ImGuiInputTextFlags kStationAddInputFlags
        = ImGuiInputTextFlags_EnterReturnsTrue
        | ImGuiInputTextFlags_AutoSelectAll
        | ImGuiInputTextFlags_CharsUppercase;

public:
    static void Draw(
        bool isVoiceConnected, const std::function<void(std::string)>& addCallback)
    {
        ImGui::PushItemWidth(-1.0);
        ImGui::Text("Add station");

        style::push_disabled_on(!isVoiceConnected);
        if (ImGui::InputText("Callsign##Auto",
                &AddStationWidget::mStationCallsignInputString, kStationAddInputFlags)
            || ImGui::Button("Add", ImVec2(-FLT_MIN, 0.0))) {
            if (isVoiceConnected) {
                std::invoke(addCallback, mStationCallsignInputString);
                AddStationWidget::mStationCallsignInputString.clear();
            }
        }
        ImGui::PopItemWidth();
        style::pop_disabled_on(!isVoiceConnected);
    }
};
}
