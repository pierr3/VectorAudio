#include "application.h"


namespace afv_unix::application {

    App::App() {
        try {
            mClient = new afv_native::api::atcClient(shared::client_name, afv_unix::configuration::get_resource_folder());
            spdlog::info("Created afv_native client.");
        } catch (std::exception ex) {
            spdlog::critical("Could not create AFV client interface: %s", ex.what());
            return;
        }

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
            afv_unix::shared::configSpeakerDeviceName = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "speaker_device", std::string(""));

            afv_unix::shared::hardware = static_cast<afv_native::HardwareType>(toml::find_or<int>(afv_unix::configuration::config, "audio", "hardware_type", 0));

            afv_unix::shared::apiServerPort = toml::find_or<int>(afv_unix::configuration::config, "general", "api_port", 49080);
        } catch (toml::exception &exc) {
            spdlog::error("Failed to parse available configuration: {}", exc.what());
        }
        
        // Bind the callbacks from the client
        mClient->RaiseClientEvent(std::bind(&App::_eventCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // Start the API timer
        shared::currentlyTransmittingApiTimer = std::chrono::high_resolution_clock::now();

        // Start the SDK server
        _buildSDKServer();
    }

    App::~App() {
        delete mClient;
    }

    void App::_buildSDKServer() {
        try {
            mSDKServer = restinio::run_async<>(
                restinio::own_io_context(),
                restinio::server_settings_t<>{}
                .port(afv_unix::shared::apiServerPort)
                .address("0.0.0.0")
                .request_handler([&](auto req) {
                    if( restinio::http_method_get() == req->header().method() && req->header().request_target() == "/transmitting" ) {
                        // Nastiest of nasty gross anti-multithread concurrency occurs here, good enough I guess
                        return req->create_response().set_body(afv_unix::shared::currentlyTransmittingApiData).done();
                    } else {
                        return req->create_response().set_body(afv_unix::shared::client_name).done(); 
                    }
                }), 1u);
        } catch(std::exception ex) {
            spdlog::error("Failed to created SDK http server, is the port in use?");
            spdlog::error("%s", ex.what());
        }
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

        if (evt == afv_native::ClientEventType::StationDataReceived) {
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
                    showWarningStationNotFound = true;
                    spdlog::warn("Station not found in AFV database through search");
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

            if (mClient->IsAPIConnected() && shared::FetchedStations.size() == 0 && !shared::bootUpVccs) {
                // We force add the current user frequency
                shared::bootUpVccs = true;

                // We replaced double _ which may be used during frequency handovers, but are not defined in database
                std::string cleanCallsign = afv_unix::util::ReplaceString(shared::datafile::callsign, "__", "_");

                shared::StationElement el = shared::StationElement::build(cleanCallsign, shared::datafile::frequency);
                if(!_frequencyExists(el.freq))
                        shared::FetchedStations.push_back(el);

                this->mClient->AddFrequency(shared::datafile::frequency, cleanCallsign);
                this->mClient->UseTransceiversFromStation(cleanCallsign, shared::datafile::frequency);
                this->mClient->SetRx(shared::datafile::frequency, true);
                if (shared::datafile::facility > 0) {
                    this->mClient->SetTx(shared::datafile::frequency, true);
                    this->mClient->SetXc(shared::datafile::frequency, true);
                }
                this->mClient->FetchStationVccs(cleanCallsign);
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
        
        // The live Received callsign data
        std::vector<std::string> ReceivedCallsigns;
        std::vector<std::string> LiveReceivedCallsigns;

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
                mClient->SetAudioSpeakersOutputDevice(afv_unix::shared::configSpeakerDeviceName);
                mClient->SetHardware(afv_unix::shared::hardware);
            
                // TODO: Pull from datafile
                mClient->SetClientPosition(48.967860, 2.442000, 100, 100);

                mClient->SetCredentials(std::to_string(afv_unix::shared::vatsim_cid), afv_unix::shared::vatsim_password);
                mClient->SetCallsign(afv_unix::shared::datafile::callsign);
                mClient->SetEnableInputFilters(afv_unix::shared::mInputFilter);
                mClient->SetEnableOutputEffects(afv_unix::shared::mOutputEffects);

                if (!mClient->Connect()) {
                    spdlog::error("Failled to connect");
                };
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

                // Cleanup everything
                for (auto f : shared::FetchedStations)
                    mClient->RemoveFrequency(f.freq);

                shared::FetchedStations.clear();
                shared::bootUpVccs = false;
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
        
        if (ImGui::Button("Settings") && !mClient->IsVoiceConnected()) {
            // Update all available data
            afv_unix::shared::availableAudioAPI = mClient->GetAudioApis();
            afv_unix::shared::availableInputDevices = mClient->GetAudioInputDevices(afv_unix::shared::mAudioApi);
            afv_unix::shared::availableOutputDevices = mClient->GetAudioOutputDevices(afv_unix::shared::mAudioApi);
            ImGui::OpenPopup("Settings Panel");
        }
            

        if (mClient->IsVoiceConnected()) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        
        afv_unix::modals::settings::render(mClient);

        ImGui::SameLine();

        const ImVec4 red(1.0, 0.0, 0.0, 1.0), green(0.0, 1.0, 0.0, 1.0);
        ImGui::TextColored(mClient->IsAPIConnected() ? green : red, "API");ImGui::SameLine(); ImGui::Text("|"); ImGui::SameLine();
        ImGui::TextColored(mClient->IsVoiceConnected() ? green : red, "Voice"); 

        /*ImGui::SameLine(); ImGui::Text(" | ");  ImGui::SameLine();

        ImGui::PushItemWidth(200.f);
        if (ImGui::SliderFloat("Radio Gain", &shared::RadioGain, 0.0f, 2.0f)) {
            if (mClient->IsVoiceConnected())
                mClient->SetRadiosGain(shared::RadioGain);
        }
        ImGui::PopItemWidth();*/

        ImGui::NewLine();

        //
        // Main area
        //

        ImGui::BeginGroup();
        ImGuiTableFlags flags =  ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("stations_table", 3, flags, ImVec2(ImGui::GetContentRegionAvailWidth()*0.8f, 0.0f)))
        {
            int counter = -1;
            for(auto &el : shared::FetchedStations) {

                if (counter == -1 || counter == 4) {
                    counter = 1;
                    ImGui::TableNextRow();
                }
                ImGui::TableSetColumnIndex(counter-1);

                float HalfHeight = ImGui::GetContentRegionAvailWidth()*0.25f;
                ImVec2 HalfSize = ImVec2(ImGui::GetContentRegionAvailWidth()*0.50f, HalfHeight);
                ImVec2 QuarterSize = ImVec2(ImGui::GetContentRegionAvailWidth()*0.25f, HalfHeight);
                
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(14, 17, 22));

                // Polling all data
                bool freqActive = mClient->IsFrequencyActive(el.freq);
                bool rxState = mClient->GetRxState(el.freq);
                bool rxActive = mClient->GetRxActive(el.freq);
                bool txState = mClient->GetTxState(el.freq);
                bool txActive = mClient->GetTxActive(el.freq);
                bool xcState = mClient->GetXcState(el.freq);
                bool isOnSpeaker = !mClient->GetOnHeadset(el.freq);

                //
                // Frequency button
                //
                if (freqActive)
                    afv_unix::style::button_green();
                // Disable the hover colour for this item
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(14, 17, 22));
                size_t callsignSize = el.callsign.length()/2;
                std::string padded_freq = std::string(callsignSize - std::min(callsignSize, el.human_freq.length()/2), ' ') + el.human_freq;
                std::string btnText = el.callsign + "\n" + padded_freq;
                if (ImGui::Button(btnText.c_str(), HalfSize)) 
                    ImGui::OpenPopup(el.callsign.c_str());
                ImGui::SameLine(0.f, 0.01f);
                ImGui::PopStyleColor();

                //
                // Frequency management popup
                //
                if (ImGui::BeginPopup(el.callsign.c_str()))
                {
                    ImGui::TextUnformatted(el.callsign.c_str());
                    ImGui::Separator();
                    if (ImGui::Selectable(std::string("Force Refresh##").append(el.callsign).c_str())) 
                    {
                        mClient->FetchTransceiverInfo(el.callsign);
                    }
                    if (ImGui::Selectable(std::string("Delete##").append(el.callsign).c_str()))
                    {
                        shared::StationsPendingRemoval.push_back(el.freq);
                    }
                    ImGui::EndPopup();
                }

                if (freqActive)
                    afv_unix::style::button_reset_colour();

                //
                // RX Button
                //
                if (rxState) 
                {
                    // Set button colour
                    rxActive ? afv_unix::style::button_yellow() : afv_unix::style::button_green();

                    auto receivedCld = mClient->LastTransmitOnFreq(el.freq);
                    if (receivedCld.size() > 0 && std::find(ReceivedCallsigns.begin(), ReceivedCallsigns.end(), receivedCld) == ReceivedCallsigns.end()) {
                        ReceivedCallsigns.push_back(receivedCld);
                    }

                    // Here we filter not the last callsigns that transmitted, but only the ones that are currently transmitting
                    if (rxActive && receivedCld.size() > 0 && std::find(LiveReceivedCallsigns.begin(), LiveReceivedCallsigns.end(), receivedCld) == LiveReceivedCallsigns.end()) {
                        LiveReceivedCallsigns.push_back(receivedCld);
                    }
                }
                
                if (ImGui::Button(std::string("RX##").append(el.callsign).c_str(), HalfSize)) 
                {
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

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.f);

                // New line

                //
                // XC
                //

                if (xcState)
                    afv_unix::style::button_green();

                if (ImGui::Button(std::string("XC##").append(el.callsign).c_str(), QuarterSize) && shared::datafile::facility > 0) 
                {
                    if (freqActive) {
                        mClient->SetXc(el.freq, !xcState);
                    }
                    else {
                        mClient->AddFrequency(el.freq, el.callsign);
                        mClient->UseTransceiversFromStation(el.callsign, el.freq);
                        mClient->SetTx(el.freq, true);
                        mClient->SetRx(el.freq, true);
                        mClient->SetXc(el.freq, true);
                    }
                } 

                if (xcState)
                    afv_unix::style::button_reset_colour();

                ImGui::SameLine(0.f, 0.01f);

                //
                // Speaker device
                //
                
                if (isOnSpeaker)
                    afv_unix::style::button_green();

                std::string transceiverCount = std::to_string(el.transceivers);
                if (transceiverCount.length() == 1)
                    transceiverCount = " " + transceiverCount;

                std::string speakerString = el.transceivers == -1 ? "XX" : transceiverCount;
                speakerString.append("\nSPK##");
                speakerString.append(el.callsign);
                if (ImGui::Button(speakerString.c_str(), QuarterSize)) 
                {
                    if (freqActive)
                        mClient->SetOnHeadset(el.freq, isOnSpeaker ? true : false);
                }
                
                if (isOnSpeaker)
                    afv_unix::style::button_reset_colour();

                ImGui::SameLine(0.f, 0.01f);

                //
                // TX
                //

                if (txState)
                    txActive ? afv_unix::style::button_yellow() : afv_unix::style::button_green();

                if (ImGui::Button(std::string("TX##").append(el.callsign).c_str(), HalfSize) && shared::datafile::facility > 0) {
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


                ImGui::PopStyleColor();
                ImGui::PopStyleVar(2);

                counter++;
            }

            ImGui::EndTable();
        }
        ImGui::EndGroup();

        //
        // Side pannel settings
        //

        ImGui::SameLine();

        ImGui::BeginGroup();

        ImGui::PushItemWidth(-1.0f);
        ImGui::Text("Add station");
        ImGui::InputText("Callsign##Auto", &shared::station_auto_add_callsign);
        ImGui::PopItemWidth();

        if (ImGui::Button("Add", ImVec2(-FLT_MIN, 0.0f))) {
            if (mClient->IsVoiceConnected()) {
                mClient->GetStation(shared::station_auto_add_callsign);
                mClient->FetchStationVccs(shared::station_auto_add_callsign);
                shared::station_auto_add_callsign = "";
            }
        }

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


        std::string RxList = "Last RX: ";
        RxList.append(ReceivedCallsigns.empty() ? "" : std::accumulate( ++ReceivedCallsigns.begin(), ReceivedCallsigns.end(), *ReceivedCallsigns.begin(), 
            [](auto& a, auto& b) { return a + ", " + b; }));
        ImGui::PushItemWidth(-1.0f);
        ImGui::TextWrapped("%s", RxList.c_str());
        ImGui::PopItemWidth();

        ImGui::NewLine();

        ImGui::TextUnformatted(afv_unix::shared::client_name.c_str());

        util::TextURL("Licenses", afv_unix::configuration::get_resource_folder() + "LICENSE.txt");

        ImGui::EndGroup();


        // Clear out the old API data every 500ms
        auto current_time = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(current_time - shared::currentlyTransmittingApiTimer).count() >= 500) {
            shared::currentlyTransmittingApiData = "";

            shared::currentlyTransmittingApiData.append(LiveReceivedCallsigns.empty() ? "" : std::accumulate( ++LiveReceivedCallsigns.begin(), LiveReceivedCallsigns.end(), *LiveReceivedCallsigns.begin(), 
            [](auto& a, auto& b) { return a + "," + b; }));
            shared::currentlyTransmittingApiTimer = current_time;
        }

        ImGui::End();

    }

}