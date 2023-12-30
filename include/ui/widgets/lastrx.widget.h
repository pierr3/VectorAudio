#pragma once
#include "afv-native/atcClientWrapper.h"
#include "imgui.h"
#include "shared.h"
#include "style.h"

#include <numeric>
#include <string>
#include <vector>

namespace vector_audio::ui::widgets {
class LastRxWidget {

public:
    static void Draw(const std::vector<std::string>& receivedCallsigns)
    {
        std::string rxList = "Last RX: ";
        rxList.append(receivedCallsigns.empty()
                ? ""
                : std::accumulate(++receivedCallsigns.begin(),
                    receivedCallsigns.end(), *receivedCallsigns.begin(),
                    [](auto& a, auto& b) { return a + ", " + b; }));
        ImGui::PushItemWidth(-1.0);
        ImGui::TextWrapped("%s", rxList.c_str());
        ImGui::PopItemWidth();
    }
};
}
