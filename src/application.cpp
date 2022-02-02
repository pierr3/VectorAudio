#include "application.h"


namespace afv_unix::application {

    App::App() {
        mClient = new afv_native::api::atcClient("AFV Unix Alpha");

        // Load all from config
        
        afv_unix::shared::mOutputEffects = toml::find_or<bool>(afv_unix::configuration::config, "audio", "vhf_effects", true);
        afv_unix::shared::mInputFilter = toml::find_or<bool>(afv_unix::configuration::config, "audio", "input_filters", true);
 
        afv_unix::shared::vatsim_cid = toml::find_or<int>(afv_unix::configuration::config, "user", "vatsim_id", 999999);
        afv_unix::shared::vatsim_password = toml::find_or<std::string>(afv_unix::configuration::config, "user", "vatsim_password", std::string("password"));

        auto mAudioProviders = mClient->GetAudioApis();
        afv_unix::shared::configAudioApi = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "api", std::string("Default API"));
        for(const auto& driver : mAudioProviders)
        {
            if (driver.second == afv_unix::shared::configAudioApi)
                afv_unix::shared::mAudioApi = driver.first;
        }

        afv_unix::shared::configInputDeviceName = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "input_device", std::string(""));
        afv_unix::shared::configOutputDeviceName = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "output_device", std::string(""));
    }

    App::~App() {
        delete mClient;
    }

    // Main loop
    void App::render_frame() {

        if (mClient) {
            afv_unix::shared::mPeak = mClient->GetInputPeak();
            afv_unix::shared::mVu = mClient->GetInputVu();
        }

        #ifdef IMGUI_HAS_VIEWPORT
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->GetWorkPos());
            ImGui::SetNextWindowSize(viewport->GetWorkSize());
            ImGui::SetNextWindowViewport(viewport->ID);
        #else 
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        #endif

        ImGui::Begin("MainWindow", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
        
        // Callsign Field
        ImGui::PushItemWidth(100.0f);
        ImGui::Text(std::string("Callsign: ").append(shared::callsign).c_str());
        ImGui::PopItemWidth();
        ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
        ImGui::SameLine();

        // Connect button logic

        if (!mClient->IsVoiceConnected()) {
            if (ImGui::Button("Connect")) {
                mClient->StopAudio();
                mClient->SetAudioApi(afv_unix::shared::mAudioApi);
                mClient->SetAudioInputDevice(afv_unix::shared::configInputDeviceName);
                mClient->SetAudioOutputDevice(afv_unix::shared::configOutputDeviceName);
            
                // TODO: Pull from datafile
                mClient->SetClientPosition(48.967860, 2.442000, 100, 100);

                mClient->SetCredentials(std::to_string(afv_unix::shared::vatsim_cid), afv_unix::shared::vatsim_password);
                mClient->SetCallsign(afv_unix::shared::callsign);
                mClient->SetEnableInputFilters(afv_unix::shared::mInputFilter);
                mClient->SetEnableOutputEffects(afv_unix::shared::mOutputEffects);

                if (!mClient->Connect()) {
                    std::cout << "Connection failed!" << std::endl;
                };
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(4 / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(4 / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(4 / 7.0f, 0.8f, 0.8f));
            if (ImGui::Button("Disconnect")) {
                mClient->Disconnect();
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();

        // Settings modal
        if (mClient->IsVoiceConnected()) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        
        if (ImGui::Button("Settings") && !mClient->IsVoiceConnected())
            ImGui::OpenPopup("Settings Panel");

        if (mClient->IsVoiceConnected()) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        
        afv_unix::modals::settings::render(mClient);

        ImGui::SameLine();

        const ImVec4 red(1.0, 0.0, 0.0, 1.0), green(0.0, 1.0, 0.0, 1.0);
        ImGui::TextColored(mClient->IsAPIConnected() ? green : red, "API");ImGui::SameLine(); ImGui::Text(" | "); ImGui::SameLine();
        ImGui::TextColored(mClient->IsVoiceConnected() ? green : red, "Voice"); 

        ImGui::NewLine();

        //
        // Main area
        //

        ImGui::BeginGroup();
        ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("stations_table", 8, flags, ImVec2(ImGui::GetContentRegionAvailWidth()*0.8f, 0.0f)))
        {
            // Display headers so we can inspect their interaction with borders.
            // (Headers are not the main purpose of this section of the demo, so we are not elaborating on them too much. See other sections for details)

            ImGui::TableSetupColumn("Callsign");
            ImGui::TableSetupColumn("Frequency");
            ImGui::TableSetupColumn("Transceivers");
            ImGui::TableSetupColumn("Main");
            ImGui::TableSetupColumn("RX");
            ImGui::TableSetupColumn("TX");
            ImGui::TableSetupColumn("XC");
            ImGui::TableSetupColumn("R");
            ImGui::TableHeadersRow();
            
            for(auto &el : shared::FetchedStations) {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(el.callsign.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(std::to_string(el.freq/1000).c_str());

                ImGui::TableNextColumn();
                // If we don't have a transceiver count, we try to fetch it
                if (el.transceivers < 0) {
                    int remoteCount = mClient->GetTransceiverCountForStation(el.callsign);
                    if (remoteCount > 0) {
                        el.transceivers = remoteCount;
                    }
                }
                ImGui::TextUnformatted(std::to_string(el.transceivers).c_str());

                ImGui::TableNextColumn();
                if (ImGui::Button("Toggle", ImVec2(-FLT_MIN, 0.0f))) {
                    mClient->UseTransceiversFromStation(el.callsign, el.freq);
                }

                ImGui::TableNextColumn();

                bool rxState = mClient->GetRxState(el.freq);
                if (rxState) {
                    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(2 / 7.0f, 0.6f, 0.6f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(2 / 7.0f, 0.7f, 0.7f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(2 / 7.0f, 0.8f, 0.8f));
                }
                if (ImGui::Button("RX")) {
                    mClient->SetRx(el.freq, !rxState);
                }
                if (rxState)
                    ImGui::PopStyleColor(3);

                bool txState = mClient->GetTxState(el.freq);
                if (txState) {
                    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(2 / 7.0f, 0.6f, 0.6f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(2 / 7.0f, 0.7f, 0.7f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(2 / 7.0f, 0.8f, 0.8f));
                }
                ImGui::TableNextColumn();

                if (ImGui::Button("TX")) {
                    mClient->SetTx(el.freq, !txState);
                }
                if (txState)
                    ImGui::PopStyleColor(3);

                ImGui::TableNextColumn();
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                if (ImGui::Button("XC")) {
                    //TODO: Implement XC
                }
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();

                ImGui::TableNextColumn();
                if (ImGui::Button("X")) {
                    //mClient->RemoveFrequency(el.freq);
                    //TODO: Actually delete it
                }

                ImGui::TableNextRow();
            }

            ImGui::EndTable();
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();

        ImGui::PushItemWidth(-1.0f);
        ImGui::Text("Fetch station");
        ImGui::InputText("Callsign", &shared::station_add_callsign);
        ImGui::InputFloat("Frequency", &shared::station_add_frequency, 0.025f, 0.0f);
        ImGui::PopItemWidth();

        if (ImGui::Button("Add", ImVec2(-FLT_MIN, 0.0f))) {
            if (mClient->IsVoiceConnected()) {
                shared::StationElement el;
                el.callsign = shared::station_add_callsign;
                el.freq = shared::station_add_frequency*1000000;
                shared::FetchedStations.push_back(el);

                mClient->AddFrequency(el.freq);
                mClient->FetchTransceiverInfo(el.callsign);

                shared::station_add_callsign = "";
                shared::station_add_frequency = 118.0f;
            }
        }

        ImGui::EndGroup();

        ImGui::End();

    }

}