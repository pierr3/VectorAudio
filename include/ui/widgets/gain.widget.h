#pragma once
#include "afv-native/atcClientWrapper.h"
#include "imgui.h"
#include "shared.h"
#include "style.h"

#include <string>
#include <vector>

namespace vector_audio::ui::widgets {
class GainWidget {

public:
    static void Draw(afv_native::api::atcClient* pClient)
    {
        ImGui::PushItemWidth(-1.0);
        ImGui::Text("Radio Gain");
        style::push_disabled_on(!pClient->IsVoiceConnected());
        if (ImGui::SliderInt(
                "##Radio Gain", &shared::radioGain, 0, 200, "%.3i %%")) {
            if (pClient->IsVoiceConnected())
                pClient->SetRadioGainAll(shared::radioGain / 100.0F);
        }
        ImGui::PopItemWidth();
        style::pop_disabled_on(!pClient->IsVoiceConnected());
    }
};
}
