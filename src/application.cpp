#include "application.h"


namespace afv_unix::application {

    App::App() {
        mClient = new afv_native::api::atcClient(shared::client_name, afv_unix::configuration::get_resource_folder());

        spdlog::info("Created afv_native client.");

        // Load all from config
        try {
            afv_unix::shared::mOutputEffects = toml::find_or<bool>(afv_unix::configuration::config, "audio", "vhf_effects", true);
            afv_unix::shared::mInputFilter = toml::find_or<bool>(afv_unix::configuration::config, "audio", "input_filters", true);
    
            afv_unix::shared::vatsim_cid = toml::find_or<int>(afv_unix::configuration::config, "user", "vatsim_id", 999999);
            afv_unix::shared::vatsim_password = toml::find_or<std::string>(afv_unix::configuration::config, "user", "vatsim_password", std::string("password"));

            afv_unix::shared::ptt = static_cast<sf::Keyboard::Key>(toml::find_or<int>(afv_unix::configuration::config, "user", "ptt", -1));

            auto mAudioProviders = mClient->GetAudioApis();
            afv_unix::shared::configAudioApi = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "api", std::string("Default API"));
            for(const auto& driver : mAudioProviders)
            {
                if (driver.second == afv_unix::shared::configAudioApi)
                    afv_unix::shared::mAudioApi = driver.first;
            }

            afv_unix::shared::configInputDeviceName = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "input_device", std::string(""));
            afv_unix::shared::configOutputDeviceName = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "output_device", std::string(""));
        } catch (toml::exception &exc) {
            spdlog::error("Failed to parse available configuration: {}", exc.what());
        }
        
        // Bind the callbacks from the client
        mClient->RaiseClientEvent(std::bind(&App::_eventCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    App::~App() {
        delete mClient;
    }

    void App::_eventCallback(afv_native::ClientEventType evt, void* data, void* data2) {
        if (evt == afv_native::ClientEventType::VccsReceived) {
            if (data != nullptr && data2 != nullptr) {
                // We got new VCCS stations, we can add them to our list and start getting their transceivers
                std::map<std::string, unsigned int> stations = *reinterpret_cast<std::map<std::string, unsigned int>*>(data2);

                if (mClient->IsVoiceConnected()) {

                    for (auto s : stations) {
                        if (s.second % 25000 != 0)
                            s.second += 5000;
                        shared::StationElement el = shared::StationElement::build(s.first, s.second);

                        if(!_frequencyExists(el.freq))
                            shared::FetchedStations.push_back(el);
                    }
                }
            }
        }

        if (evt == afv_native::ClientEventType::StationTransceiversUpdated) {
            if (data != nullptr) {
                // We just refresh the transceiver count in our display
                std::string station = *reinterpret_cast<std::string*>(data);
                auto it = std::find_if(shared::FetchedStations.begin(), shared::FetchedStations.end(), [station](const auto &fs){
                    return fs.callsign == station;
                });
                if (it != shared::FetchedStations.end())
                    it->transceivers = mClient->GetTransceiverCountForStation(station);
            }
        }

        if (evt == afv_native::ClientEventType::StationSearchReceived) {
            if (data != nullptr && data2 != nullptr) {
                // We just refresh the transceiver count in our display
                bool found = *reinterpret_cast<bool*>(data);
                if (found) {
                    auto station = *reinterpret_cast<std::pair<std::string, unsigned int>*>(data2);

                    if (station.second % 25000 != 0)
                        station.second += 5000;

                    shared::StationElement el = shared::StationElement::build(station.first, station.second);
                    
                    if(!_frequencyExists(el.freq))
                        shared::FetchedStations.push_back(el);
                } else {
                    // TODO: Prompt user the station was not found
                }
            }
        }
    }

    // Main loop
    void App::render_frame() {


        // AFV stuff
        if (mClient) {
            afv_unix::shared::mPeak = mClient->GetInputPeak();
            afv_unix::shared::mVu = mClient->GetInputVu();

            // Set the Ptt if required
            if (mClient->IsVoiceConnected() && shared::ptt != sf::Keyboard::Unknown) {
                if (shared::isPttOpen) {
                    if (!sf::Keyboard::isKeyPressed(shared::ptt)) {
                        mClient->SetPtt(false);
                        shared::isPttOpen = false;
                    }
                } else {
                    if (sf::Keyboard::isKeyPressed(shared::ptt)) {
                        mClient->SetPtt(true);
                        shared::isPttOpen = true;
                    }
                }
            }
        }

        // Forcing removal of unused stations if possible, otherwise we try at the next loop
        shared::StationsPendingRemoval.erase(
            std::remove_if(shared::StationsPendingRemoval.begin(), shared::StationsPendingRemoval.end(),
                [this](int const & station) { 

                    // First we check if we are not receiving or transmitting on the frequency
                    if (!this->mClient->GetRxActive(station) && !this->mClient->GetTxActive(station)) {
                        // The frequency is free, we can remove it

                        shared::FetchedStations.erase(
                            std::remove_if(shared::FetchedStations.begin(), shared::FetchedStations.end(),
                                [station](shared::StationElement const & p) { return station == p.freq; }
                            ), 
                            shared::FetchedStations.end()
                        ); 
                        mClient->RemoveFrequency(station);

                        return true;
                    } else {
                        //The frequency is not free, we try again later
                        return false;
                    }
                }
            ), 
            shared::StationsPendingRemoval.end()
        ); 

        // Changing RX status that is locked
        shared::StationsPendingRxChange.erase(std::remove_if(shared::StationsPendingRxChange.begin(), shared::StationsPendingRxChange.end(),
        [this](int const & station) { 
            if (!this->mClient->GetRxActive(station)) {
                // Frequency is free, we can change the state
                this->mClient->SetRx(station, !this->mClient->GetRxState(station));
                return true;
            } else {
                // Frequency is not free, we try again later
                return false;
            }
        }), shared::StationsPendingRxChange.end());
        

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("MainWindow", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        
        // Callsign Field
        ImGui::PushItemWidth(100.0f);
        ImGui::TextUnformatted(std::string("Callsign: ").append(shared::datafile::callsign).c_str());
        ImGui::PopItemWidth();
        ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
        ImGui::SameLine();

        // Connect button logic

        if (!mClient->IsVoiceConnected()) {
            if (!afv_unix::shared::datafile::is_connected) {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            if (ImGui::Button("Connect") && afv_unix::shared::datafile::is_connected) {
                mClient->StopAudio();
                mClient->SetAudioApi(afv_unix::shared::mAudioApi);
                mClient->SetAudioInputDevice(afv_unix::shared::configInputDeviceName);
                mClient->SetAudioOutputDevice(afv_unix::shared::configOutputDeviceName);
            
                // TODO: Pull from datafile
                mClient->SetClientPosition(48.967860, 2.442000, 100, 100);

                mClient->SetCredentials(std::to_string(afv_unix::shared::vatsim_cid), afv_unix::shared::vatsim_password);
                mClient->SetCallsign(afv_unix::shared::datafile::callsign);
                mClient->SetEnableInputFilters(afv_unix::shared::mInputFilter);
                mClient->SetEnableOutputEffects(afv_unix::shared::mOutputEffects);

                if (!mClient->Connect()) {
                    spdlog::error("Failled to connect");
                };

                // We pre-fetch the user station
                std::string strippedCallsign = shared::datafile::callsign.substr(0, shared::datafile::callsign.find("_"));
                mClient->SearchStation(strippedCallsign, shared::datafile::frequency);

                //shared::station_add_callsign = shared::datafile::callsign;
                //shared::station_add_frequency = shared::datafile::frequency/1000000.0f;
            }
            if (!afv_unix::shared::datafile::is_connected) {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(4 / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(4 / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(4 / 7.0f, 0.8f, 0.8f));

            // Auto disconnect if we need
            if (ImGui::Button("Disconnect") || !afv_unix::shared::datafile::is_connected) {
                if (mClient->IsAtisPlayingBack())
                    mClient->StopAtisPlayback();
                mClient->Disconnect();
            }
            ImGui::PopStyleColor(3);

            // If we're connected and the ATIS is not playing playing back, we add the frequency
            if (!mClient->IsAtisPlayingBack() && afv_unix::shared::datafile::is_atis_connected) {
                mClient->StartAtisPlayback(shared::datafile::atis_callsign, shared::datafile::atis_frequency);
            }

            // Disconnect the ATIS if it disconnected
            if (mClient->IsAtisPlayingBack() && !afv_unix::shared::datafile::is_atis_connected) {
                mClient->StopAtisPlayback();
            }
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
        ImGui::TextColored(mClient->IsAPIConnected() ? green : red, "API");ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
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

            ImGui::TableSetupColumn("Callsign", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Frequency");
            ImGui::TableSetupColumn("TCS");
            ImGui::TableSetupColumn("STS");
            ImGui::TableSetupColumn("RX");
            ImGui::TableSetupColumn("TX");
            ImGui::TableSetupColumn("XC");
            ImGui::TableSetupColumn("R");
            ImGui::TableHeadersRow();
            
            for(auto &el : shared::FetchedStations) {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(el.callsign.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(el.human_freq.c_str());

                ImGui::TableNextColumn();
                // If we don't have a transceiver count, we try to fetch it
                ImGui::TextUnformatted(std::to_string(el.transceivers).c_str());

                ImGui::TableNextColumn();
                bool freqActive = mClient->IsFrequencyActive(el.freq);
                if (freqActive)
                    ImGui::TextUnformatted("LIVE");
                else
                    ImGui::TextUnformatted("INOP");

                ImGui::TableNextColumn();

                bool rxState = mClient->GetRxState(el.freq);
                bool rxActive = mClient->GetRxActive(el.freq);
                if (rxState) {
                    // Yellow colour if currently being transmitted on
                    if (rxActive)
                        afv_unix::style::button_yellow();
                    else
                        afv_unix::style::button_green();
                }
                
                if (ImGui::Button(std::string("RX##").append(el.callsign).c_str())) {
                    if (freqActive) {
                        // We check if we are receiving something, if that is the case we must wait till the end of transmition to change the state
                        if (rxActive) {
                            if (std::find(shared::StationsPendingRxChange.begin(), shared::StationsPendingRxChange.end(), el.freq) == shared::StationsPendingRxChange.end())
                                shared::StationsPendingRxChange.push_back(el.freq);
                        }
                        else {
                            mClient->SetRx(el.freq, !rxState);
                        }
                    }
                    else {
                        mClient->AddFrequency(el.freq, el.callsign);
                        mClient->UseTransceiversFromStation(el.callsign, el.freq);
                        mClient->SetRx(el.freq, true);
                    }
                }
                if (rxState)
                    afv_unix::style::button_reset_colour();

                

                ImGui::TableNextColumn();
                bool txState = mClient->GetTxState(el.freq);
                bool txActive = mClient->GetTxActive(el.freq);
                if (txState) {

                    // Yellow colour if currently being transmitted on
                    if (txActive)
                        afv_unix::style::button_yellow();
                    else
                        afv_unix::style::button_green();
                }

                if (ImGui::Button(std::string("TX##").append(el.callsign).c_str()) && shared::datafile::facility > 0 && !txActive) {
                    if (freqActive) {
                        mClient->SetTx(el.freq, !txState);
                    }
                    else {
                        mClient->AddFrequency(el.freq, el.callsign);
                        mClient->UseTransceiversFromStation(el.callsign, el.freq);
                        mClient->SetTx(el.freq, true);
                        mClient->SetRx(el.freq, true);
                    }
                }
                if (txState)
                    afv_unix::style::button_reset_colour();

                ImGui::TableNextColumn();

                bool xcState = mClient->GetXcState(el.freq);
                if (xcState)
                    afv_unix::style::button_green();

                if (ImGui::Button(std::string("XC##").append(el.callsign).c_str()) && shared::datafile::facility > 0) {
                    if (freqActive) {
                        mClient->SetXc(el.freq, !xcState);
                    }
                    else {
                        mClient->AddFrequency(el.freq, el.callsign);
                        mClient->UseTransceiversFromStation(el.callsign, el.freq);
                        if (shared::datafile::facility > 0)
                            mClient->SetTx(el.freq, true);
                        mClient->SetRx(el.freq, true);
                        mClient->SetXc(el.freq, true);
                    }
                }

                if (xcState)
                    afv_unix::style::button_reset_colour();

                ImGui::TableNextColumn();
                if (ImGui::Button(std::string("X##").append(el.callsign).c_str())) {
                    shared::StationsPendingRemoval.push_back(el.freq);
                }

                ImGui::TableNextRow();
            }

            ImGui::EndTable();
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();

        ImGui::PushItemWidth(-1.0f);
        ImGui::Text("Add station");
        ImGui::InputText("Callsign##Auto", &shared::station_auto_add_callsign);
        ImGui::PopItemWidth();

        if (ImGui::Button("Add", ImVec2(-FLT_MIN, 0.0f))) {
            if (mClient->IsVoiceConnected()) {
                mClient->SearchStation(shared::station_auto_add_callsign);
                mClient->FetchStationVccs(shared::station_auto_add_callsign);
                shared::station_auto_add_callsign = "";
            }
        }

        /*ImGui::NewLine();

        // Manual station add
        ImGui::PushItemWidth(-1.0f);
        ImGui::Text("Manual add");
        ImGui::InputText("Callsign##Manual", &shared::station_add_callsign);
        ImGui::InputFloat("Frequency", &shared::station_add_frequency, 0.025f, 0.0f);
        ImGui::PopItemWidth();

        if (ImGui::Button("Fetch", ImVec2(-FLT_MIN, 0.0f))) {
            if (mClient->IsVoiceConnected()) {
                shared::StationElement el = shared::StationElement::build(shared::station_add_callsign, shared::station_add_frequency*1000000);
                shared::FetchedStations.push_back(el);

                mClient->AddFrequency(el.freq, el.callsign);
                mClient->UseTransceiversFromStation(el.callsign, el.freq);

                shared::station_add_callsign = "";
                shared::station_add_frequency = 118.0f;
            }
        }*/

        ImGui::NewLine();

        ImGui::PushItemWidth(-1.0f);
        ImGui::Text("ATIS Status");
        ImGui::TextUnformatted(shared::datafile::atis_callsign.c_str());
        ImGui::PopItemWidth();

        if (!mClient->IsVoiceConnected() || shared::datafile::atis_callsign.size() == 0) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        bool atisPlayingBack = mClient->IsAtisPlayingBack();
        std::string bttnAtisPlayback = atisPlayingBack ? "Broadcast active" : "Broadcast stopped";
        if (atisPlayingBack)
            afv_unix::style::button_green();

        ImGui::Button(bttnAtisPlayback.c_str(), ImVec2(-FLT_MIN, 0.0f));

        //mClient->StartAtisPlayback(shared::datafile::atis_callsign, shared::datafile::atis_frequency);
        //mClient->StopAtisPlayback();

        if (atisPlayingBack)
            afv_unix::style::button_reset_colour();
        
        bool listeningInAtis = mClient->IsAtisListening();
        if (listeningInAtis)
            afv_unix::style::button_yellow();

        if (ImGui::Button("Listen in", ImVec2(-FLT_MIN, 0.0f))) {
            if (mClient->IsVoiceConnected() && shared::datafile::atis_callsign.size() > 0 && atisPlayingBack) {
                mClient->SetAtisListening(!listeningInAtis);
            }
        }

        if (listeningInAtis)
            afv_unix::style::button_reset_colour();

        if (!mClient->IsVoiceConnected() || shared::datafile::atis_callsign.size() == 0) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }

        ImGui::NewLine();

        //ImGui::PushItemWidth(-1.0f);
        //ImGui::TextUnformatted("Last RX: [Not implemented]");
        //ImGui::PopItemWidth();

        ImGui::TextUnformatted(afv_unix::shared::client_name.c_str());

        ImGui::EndGroup();


        ImGui::End();

    }

}