#pragma once
#include "afv-native/atcClientWrapper.h"
#include "data_file_handler.h"
#include "imgui.h"
#include "shared.h"
#include "ui/style.h"

#include <string>
#include <vector>

namespace vector_audio::ui::widgets {
class NetworkStatusWidget {

public:
    static void Draw(bool isVoiceConnected, bool isSlurperAvailable,
        bool isDatafileAvailable)
    {

        ImGui::TextColored(isVoiceConnected ? kGreen : kRed, "API");
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        ImGui::TextColored(isVoiceConnected ? kGreen : kRed, "Voice");
        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();
        // Status about datasource

        if (isSlurperAvailable) {
            ImGui::TextColored(kGreen, "Slurper");
            /*if (ImGui::IsItemClicked()) {
                shared::slurper::is_unavailable = true;
            }*/
        } else if (isDatafileAvailable) {
            ImGui::TextColored(kYellow, "Datafile");
        } else {
            ImGui::TextColored(kRed, "No VATSIM Data");
        }

        ImGui::SameLine();

        util::HelpMarker(
            "The data source where VectorAudio\nchecks for your VATSIM "
            "connection.\n"
            "No VATSIM Data means that VATSIM servers could not be reached at "
            "all.");
    }

private:
    constexpr static const ImVec4 kRed = { 1.0, 0.0, 0.0, 1.0 };
    constexpr static const ImVec4 kYellow = ImVec4(1.0, 1.0, 0.0, 1.0);
    constexpr static const ImVec4 kGreen = ImVec4(0.0, 1.0, 0.0, 1.0);
};
}
