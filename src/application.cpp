#include "application.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "style.h"
#include "util.h"
#include <spdlog/spdlog.h>

namespace vector_audio::application {
using util::TextURL;

App::App()
{
    try {
        mClient_ = new afv_native::api::atcClient(
            shared::client_name,
            vector_audio::configuration::get_resource_folder());
        spdlog::debug("Created afv_native client.");
    } catch (std::exception& ex) {
        spdlog::critical("Could not create AFV client interface: {}", ex.what());
        return;
    }

    // Load all from config
    try {
        using cfg = vector_audio::configuration;

        vector_audio::shared::mOutputEffects = toml::find_or<bool>(cfg::config, "audio", "vhf_effects", true);
        vector_audio::shared::mInputFilter = toml::find_or<bool>(cfg::config, "audio", "input_filters", true);

        vector_audio::shared::vatsim_cid = toml::find_or<int>(cfg::config, "user", "vatsim_id", 999999);
        vector_audio::shared::vatsim_password = toml::find_or<std::string>(
            cfg::config, "user", "vatsim_password", std::string("password"));

        vector_audio::shared::ptt = static_cast<sf::Keyboard::Key>(
            toml::find_or<int>(cfg::config, "user", "ptt", -1));

        auto audio_providers = mClient_->GetAudioApis();
        vector_audio::shared::configAudioApi = toml::find_or<std::string>(
            cfg::config, "audio", "api", std::string("Default API"));
        for (const auto& driver : audio_providers) {
            if (driver.second == vector_audio::shared::configAudioApi)
                vector_audio::shared::mAudioApi = driver.first;
        }

        vector_audio::shared::configInputDeviceName = toml::find_or<std::string>(
            cfg::config, "audio", "input_device", std::string(""));
        vector_audio::shared::configOutputDeviceName = toml::find_or<std::string>(
            cfg::config, "audio", "output_device", std::string(""));
        vector_audio::shared::configSpeakerDeviceName = toml::find_or<std::string>(
            cfg::config, "audio", "speaker_device", std::string(""));

        vector_audio::shared::hardware = static_cast<afv_native::HardwareType>(
            toml::find_or<int>(cfg::config, "audio", "hardware_type", 0));

        vector_audio::shared::apiServerPort = toml::find_or<int>(cfg::config, "general", "api_port", 49080);
    } catch (toml::exception& exc) {
        spdlog::error("Failed to parse available configuration: {}", exc.what());
    }

    // Bind the callbacks from the client
    // std::bind(&App::_eventCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
    mClient_->RaiseClientEvent(
        [this](auto&& event_type, auto&& data_one, auto&& data_two) {
            eventCallback(std::forward<decltype(event_type)>(event_type),
                std::forward<decltype(data_one)>(data_one),
                std::forward<decltype(data_two)>(data_two));
        });

    // Start the API timer
    shared::currentlyTransmittingApiTimer = std::chrono::high_resolution_clock::now();

    // Start the SDK server
    buildSDKServer();

    // Load the airport database async
    std::thread(&vector_audio::application::App::loadAirportsDatabaseAsync).detach();
}

App::~App() { delete mClient_; }

void App::loadAirportsDatabaseAsync()
{
    // if we cannot load this database, it's not that important, we will just log
    // it.

    if (!std::filesystem::exists(
            vector_audio::configuration::airports_db_file_path)) {
        spdlog::warn("Could not find airport database json file");
        return;
    }

    try {
        // We do performance analysis here
        auto t1 = std::chrono::high_resolution_clock::now();
        std::ifstream f(vector_audio::configuration::airports_db_file_path);
        nlohmann::json data = nlohmann::json::parse(f);

        // Loop through all the icaos
        for (const auto& obj : data.items()) {
            ns::Airport ar;
            obj.value().at("icao").get_to(ar.icao);
            obj.value().at("elevation").get_to(ar.elevation);
            obj.value().at("lat").get_to(ar.lat);
            obj.value().at("lon").get_to(ar.lon);

            // Assumption: The user will not have time to connect by the time this is
            // loaded, hence should be fine re concurrency
            ns::Airport::All.insert(std::make_pair(obj.key(), ar));
        }

        auto t2 = std::chrono::high_resolution_clock::now();
        spdlog::info(
            "Loaded {} airports in {}", ns::Airport::All.size(),
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1));
    } catch (nlohmann::json::exception& ex) {
        spdlog::warn("Could parse airport database: {}", ex.what());
        return;
    }
}

void App::buildSDKServer()
{
    try {
        mSDKServer_ = restinio::run_async<>(
            restinio::own_io_context(),
            restinio::server_settings_t<> {}
                .port(vector_audio::shared::apiServerPort)
                .address("0.0.0.0")
                .request_handler([&](auto req) {
                    if (restinio::http_method_get() == req->header().method()
                        && req->header().request_target() == "/transmitting") {
                        // Nastiest of nasty gross anti-multithread concurrency occurs
                        // here, good enough I guess
                        return req->create_response()
                            .set_body(
                                vector_audio::shared::currentlyTransmittingApiData)
                            .done();
                    }

                    return req->create_response()
                        .set_body(vector_audio::shared::client_name)
                        .done();
                }),
            16U);
    } catch (std::exception& ex) {
        spdlog::error("Failed to created SDK http server, is the port in use?");
        spdlog::error("%{}", ex.what());
    }
}

void App::eventCallback(afv_native::ClientEventType evt, void* data,
    void* data2)
{
    if (evt == afv_native::ClientEventType::VccsReceived) {
        if (data != nullptr && data2 != nullptr) {
            // We got new VCCS stations, we can add them to our list and start getting
            // their transceivers
            std::map<std::string, unsigned int> stations = *reinterpret_cast<std::map<std::string, unsigned int>*>(data2);

            if (mClient_->IsVoiceConnected()) {
                for (auto s : stations) {
                    if (s.second % 25000 != 0)
                        s.second += 5000;
                    shared::StationElement el = shared::StationElement::build(s.first, s.second);

                    if (!frequencyExists(el.freq))
                        shared::FetchedStations.push_back(el);
                }
            }
        }
    }

    if (evt == afv_native::ClientEventType::StationTransceiversUpdated) {
        if (data != nullptr) {
            // We just refresh the transceiver count in our display
            std::string station = *reinterpret_cast<std::string*>(data);
            auto it = std::find_if(
                shared::FetchedStations.begin(), shared::FetchedStations.end(),
                [station](const auto& fs) { return fs.callsign == station; });
            if (it != shared::FetchedStations.end())
                it->transceivers = mClient_->GetTransceiverCountForStation(station);
        }
    }

    if (evt == afv_native::ClientEventType::APIServerError) {
        // We got an error from the API server, we can display this to the user
        if (data != nullptr) {
            afv_native::afv::APISessionError err = *reinterpret_cast<afv_native::afv::APISessionError*>(data);

            if (err == afv_native::afv::APISessionError::BadPassword || err == afv_native::afv::APISessionError::RejectedCredentials) {
                errorModal("Could not login to VATSIM.\nInvalid "
                           "Credentials.\nCheck your password/cid!");
                           
                spdlog::error("Got invalid credential errors from AFV API: HTTP 403 or 401");
            }

            if (err == afv_native::afv::APISessionError::ConnectionError) {
                errorModal("Could not login to VATSIM.\nConnection "
                           "Error.\nCheck your internet connection.");

                spdlog::error("Got connection error from AFV API: local socket or curl error");
            }

            if (err == afv_native::afv::APISessionError::BadRequestOrClientIncompatible) {
                errorModal("Could not login to VATSIM.\n Bad Request or Client Incompatible.");

                spdlog::error("Got connection error from AFV API: HTTP 400 - Bad Request or Client Incompatible");
            }

            if (err == afv_native::afv::APISessionError::InvalidAuthToken) {
                errorModal("Could not login to VATSIM.\n Invalid Auth Token.");

                spdlog::error("Got connection error from AFV API: Invalid Auth Token Local Parse Error.");
            }

            if (err == afv_native::afv::APISessionError::AuthTokenExpiryTimeInPast) {
                errorModal("Could not login to VATSIM.\n Auth Token has expired.\n Check your system clock.");

                spdlog::error("Got connection error from AFV API: Auth Token Expiry in the past");
            }

            if (err == afv_native::afv::APISessionError::OtherRequestError) {
                errorModal("Could not login to VATSIM.\n Unknown Error.");

                spdlog::error("Got connection error from AFV API: Unknown Error");
            }
        }
    }

    if (evt == afv_native::ClientEventType::StationDataReceived) {
        if (data != nullptr && data2 != nullptr) {
            // We just refresh the transceiver count in our display
            bool found = *reinterpret_cast<bool*>(data);
            if (found) {
                auto station = *reinterpret_cast<std::pair<std::string, unsigned int>*>(data2);

                station.second = util::cleanUpFrequency(station.second);

                shared::StationElement el = shared::StationElement::build(station.first, station.second);

                if (!frequencyExists(el.freq))
                    shared::FetchedStations.push_back(el);
            } else {
                errorModal("Could not find station in database.");
                spdlog::warn("Station not found in AFV database through search");
            }
        }
    }
}

// Main loop
void App::render_frame()
{
    // AFV stuff
    if (mClient_) {
        vector_audio::shared::mPeak = mClient_->GetInputPeak();
        vector_audio::shared::mVu = mClient_->GetInputVu();

        // Set the Ptt if required
        if (mClient_->IsVoiceConnected() && shared::ptt != sf::Keyboard::Unknown) {
            if (shared::isPttOpen) {
                if (!sf::Keyboard::isKeyPressed(shared::ptt)) {
                    mClient_->SetPtt(false);
                    shared::isPttOpen = false;
                }
            } else {
                if (sf::Keyboard::isKeyPressed(shared::ptt)) {
                    mClient_->SetPtt(true);
                    shared::isPttOpen = true;
                }
            }
        }

        if (mClient_->IsAPIConnected() && shared::FetchedStations.empty() && !shared::bootUpVccs) {
            // We force add the current user frequency
            shared::bootUpVccs = true;

            // We replaced double _ which may be used during frequency handovers, but
            // are not defined in database
            std::string clean_callsign = vector_audio::util::ReplaceString(
                shared::datafile::callsign, "__", "_");

            shared::StationElement el = shared::StationElement::build(
                clean_callsign, shared::datafile::frequency);
            if (!frequencyExists(el.freq))
                shared::FetchedStations.push_back(el);

            this->mClient_->AddFrequency(shared::datafile::frequency, clean_callsign);
            this->mClient_->UseTransceiversFromStation(clean_callsign,
                shared::datafile::frequency);
            this->mClient_->SetRx(shared::datafile::frequency, true);
            if (shared::datafile::facility > 0) {
                this->mClient_->SetTx(shared::datafile::frequency, true);
                this->mClient_->SetXc(shared::datafile::frequency, true);
            }
            this->mClient_->FetchStationVccs(clean_callsign);
        }
    }

    // Forcing removal of unused stations if possible, otherwise we try at the
    // next loop
    shared::StationsPendingRemoval.erase(
        std::remove_if(shared::StationsPendingRemoval.begin(),
            shared::StationsPendingRemoval.end(),
            [this](int const& station) {
                // First we check if we are not receiving or transmitting
                // on the frequency
                if (!this->mClient_->GetRxActive(station) && !this->mClient_->GetTxActive(station)) {
                    // The frequency is free, we can remove it

                    shared::FetchedStations.erase(
                        std::remove_if(
                            shared::FetchedStations.begin(),
                            shared::FetchedStations.end(),
                            [station](shared::StationElement const& p) {
                                return station == p.freq;
                            }),
                        shared::FetchedStations.end());
                    mClient_->RemoveFrequency(station);

                    return true;
                } // The frequency is not free, we try again later
                return false;
            }),
        shared::StationsPendingRemoval.end());

    // Changing RX status that is locked
    shared::StationsPendingRxChange.erase(
        std::remove_if(shared::StationsPendingRxChange.begin(),
            shared::StationsPendingRxChange.end(),
            [this](int const& station) {
                if (!this->mClient_->GetRxActive(station)) {
                    // Frequency is free, we can change the state
                    this->mClient_->SetRx(
                        station, !this->mClient_->GetRxState(station));
                    return true;
                } // Frequency is not free, we try again later
                return false;
            }),
        shared::StationsPendingRxChange.end());

    // The live Received callsign data
    std::vector<std::string> received_callsigns;
    std::vector<std::string> live_received_callsigns;

    ImGui::SetNextWindowPos(ImVec2(0.0F, 0.0F));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("MainWindow", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Callsign Field
    ImGui::PushItemWidth(100.0F);
    ImGui::TextUnformatted(
        std::string("Callsign: ").append(shared::datafile::callsign).c_str());
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    ImGui::SameLine();

    // Connect button logic

    if (!mClient_->IsVoiceConnected()) {
        style::push_disabled_on(!vector_audio::shared::datafile::is_connected);

        if (ImGui::Button("Connect") && vector_audio::shared::datafile::is_connected) {
            mClient_->StopAudio();
            mClient_->SetAudioApi(vector_audio::shared::mAudioApi);
            mClient_->SetAudioInputDevice(vector_audio::shared::configInputDeviceName);
            mClient_->SetAudioOutputDevice(
                vector_audio::shared::configOutputDeviceName);
            mClient_->SetAudioSpeakersOutputDevice(
                vector_audio::shared::configSpeakerDeviceName);
            mClient_->SetHardware(vector_audio::shared::hardware);

            std::string client_icao = vector_audio::shared::datafile::callsign.substr(
                0, vector_audio::shared::datafile::callsign.find('_'));
            // We use the airport database for this
            if (ns::Airport::All.find(client_icao) != ns::Airport::All.end()) {
                auto client_airport = ns::Airport::All.at(client_icao);

                // We pad the elevation by 10 meters to simulate the client being in a
                // tower
                mClient_->SetClientPosition(client_airport.lat, client_airport.lon,
                    client_airport.elevation + 33,
                    client_airport.elevation + 33);

                spdlog::info(
                    "Found client position in database at lat:{}, lon:{}, elev:{}",
                    client_airport.lat, client_airport.lon, client_airport.elevation);
            } else {
                spdlog::info("Client position is unknown, setting default.");

                // Default position is over Paris somewhere
                mClient_->SetClientPosition(48.967860, 2.442000, 100, 100);
            }

            mClient_->SetCredentials(std::to_string(vector_audio::shared::vatsim_cid),
                vector_audio::shared::vatsim_password);
            mClient_->SetCallsign(vector_audio::shared::datafile::callsign);
            mClient_->SetEnableInputFilters(vector_audio::shared::mInputFilter);
            mClient_->SetEnableOutputEffects(vector_audio::shared::mOutputEffects);
            mClient_->SetRadiosGain(shared::RadioGain/100.0F);

            if (!mClient_->Connect()) {
                spdlog::error("Failled to connect");
            };
        }
        style::pop_disabled_on(!vector_audio::shared::datafile::is_connected);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,
            ImColor::HSV(4 / 7.0F, 0.6F, 0.6F).Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            ImColor::HSV(4 / 7.0F, 0.7F, 0.7F).Value);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            ImColor::HSV(4 / 7.0F, 0.8F, 0.8F).Value);

        // Auto disconnect if we need
        if (ImGui::Button("Disconnect") || !vector_audio::shared::datafile::is_connected) {
            if (mClient_->IsAtisPlayingBack())
                mClient_->StopAtisPlayback();
            mClient_->Disconnect();

            // Cleanup everything
            for (const auto& f : shared::FetchedStations)
                mClient_->RemoveFrequency(f.freq);

            shared::FetchedStations.clear();
            shared::bootUpVccs = false;
        }
        ImGui::PopStyleColor(3);

        // If we're connected and the ATIS is not playing playing back, we add the
        // frequency
        if (!mClient_->IsAtisPlayingBack() && vector_audio::shared::datafile::is_atis_connected) {
            mClient_->StartAtisPlayback(shared::datafile::atis_callsign,
                shared::datafile::atis_frequency);
        }

        // Disconnect the ATIS if it disconnected
        if (mClient_->IsAtisPlayingBack() && !vector_audio::shared::datafile::is_atis_connected) {
            mClient_->StopAtisPlayback();
        }
    }

    ImGui::SameLine();

    // Settings modal
    style::push_disabled_on(mClient_->IsVoiceConnected());
    if (ImGui::Button("Settings") && !mClient_->IsVoiceConnected()) {
        // Update all available data
        vector_audio::shared::availableAudioAPI = mClient_->GetAudioApis();
        vector_audio::shared::availableInputDevices = mClient_->GetAudioInputDevices(vector_audio::shared::mAudioApi);
        vector_audio::shared::availableOutputDevices = mClient_->GetAudioOutputDevices(vector_audio::shared::mAudioApi);
        ImGui::OpenPopup("Settings Panel");
    }
    style::pop_disabled_on(mClient_->IsVoiceConnected());

    vector_audio::modals::settings::render(mClient_);

    {
        ImGui::SetNextWindowSize(ImVec2(300, 150));
        if (ImGui::BeginPopupModal(
                "Error", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
            vector_audio::util::TextCentered(lastErrorModalMessage_);

            ImGui::NewLine();
            if (ImGui::Button("Ok", ImVec2(-FLT_MIN, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::SameLine();

    const ImVec4 red(1.0, 0.0, 0.0, 1.0);
    const ImVec4 green(0.0, 1.0, 0.0, 1.0);
    ImGui::TextColored(mClient_->IsAPIConnected() ? green : red, "API");
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    ImGui::TextColored(mClient_->IsVoiceConnected() ? green : red, "Voice");

    ImGui::NewLine();

    //
    // Main area
    //

    ImGui::BeginGroup();
    ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable(
            "stations_table", 3, flags,
            ImVec2(ImGui::GetContentRegionAvailWidth() * 0.8F, 0.0F))) {
        int counter = -1;
        for (auto& el : shared::FetchedStations) {
            if (counter == -1 || counter == 4) {
                counter = 1;
                ImGui::TableNextRow();
            }
            ImGui::TableSetColumnIndex(counter - 1);

            float half_height = ImGui::GetContentRegionAvailWidth() * 0.25F;
            ImVec2 half_size = ImVec2(ImGui::GetContentRegionAvailWidth() * 0.50F, half_height);
            ImVec2 quarter_size = ImVec2(ImGui::GetContentRegionAvailWidth() * 0.25F, half_height);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.F);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.F);
            ImGui::PushStyleColor(ImGuiCol_Button, ImColor(14, 17, 22).Value);

            // Polling all data
            bool freq_active = mClient_->IsFrequencyActive(el.freq);
            bool rx_state = mClient_->GetRxState(el.freq);
            bool rx_active = mClient_->GetRxActive(el.freq);
            bool tx_state = mClient_->GetTxState(el.freq);
            bool tx_active = mClient_->GetTxActive(el.freq);
            bool xc_state = mClient_->GetXcState(el.freq);
            bool is_on_speaker = !mClient_->GetOnHeadset(el.freq);

            //
            // Frequency button
            //
            if (freq_active)
                vector_audio::style::button_green();
            // Disable the hover colour for this item
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImColor(14, 17, 22).Value);
            size_t callsign_size = el.callsign.length() / 2;
            std::string padded_freq = std::string(
                                          callsign_size - std::min(callsign_size, el.human_freq.length() / 2),
                                          ' ')
                + el.human_freq;
            std::string btn_text = el.callsign + "\n" + padded_freq;
            if (ImGui::Button(btn_text.c_str(), half_size))
                ImGui::OpenPopup(el.callsign.c_str());
            ImGui::SameLine(0, 0.01);
            ImGui::PopStyleColor();

            //
            // Frequency management popup
            //
            if (ImGui::BeginPopup(el.callsign.c_str())) {
                ImGui::TextUnformatted(el.callsign.c_str());
                ImGui::Separator();
                if (ImGui::Selectable(
                        std::string("Force Refresh##").append(el.callsign).c_str())) {
                    mClient_->FetchTransceiverInfo(el.callsign);
                }
                if (ImGui::Selectable(
                        std::string("Delete##").append(el.callsign).c_str())) {
                    shared::StationsPendingRemoval.push_back(el.freq);
                }
                ImGui::EndPopup();
            }

            if (freq_active)
                vector_audio::style::button_reset_colour();

            //
            // RX Button
            //
            if (rx_state) {
                // Set button colour
                rx_active ? vector_audio::style::button_yellow()
                          : vector_audio::style::button_green();

                auto received_cld = mClient_->LastTransmitOnFreq(el.freq);
                if (!received_cld.empty() && std::find(received_callsigns.begin(), received_callsigns.end(), received_cld) == received_callsigns.end()) {
                    received_callsigns.push_back(received_cld);
                }

                // Here we filter not the last callsigns that transmitted, but only the
                // ones that are currently transmitting
                if (rx_active && !received_cld.empty() && std::find(live_received_callsigns.begin(), live_received_callsigns.end(), received_cld) == live_received_callsigns.end()) {
                    live_received_callsigns.push_back(received_cld);
                }
            }

            if (ImGui::Button(std::string("RX##").append(el.callsign).c_str(),
                    half_size)) {
                if (freq_active) {
                    // We check if we are receiving something, if that is the case we must
                    // wait till the end of transmition to change the state
                    if (rx_active) {
                        if (std::find(shared::StationsPendingRxChange.begin(),
                                shared::StationsPendingRxChange.end(),
                                el.freq)
                            == shared::StationsPendingRxChange.end())
                            shared::StationsPendingRxChange.push_back(el.freq);
                    } else {
                        mClient_->SetRx(el.freq, !rx_state);
                    }
                } else {
                    mClient_->AddFrequency(el.freq, el.callsign);
                    mClient_->UseTransceiversFromStation(el.callsign, el.freq);
                    mClient_->SetRx(el.freq, true);
                }
            }

            if (rx_state)
                vector_audio::style::button_reset_colour();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);

            // New line

            //
            // XC
            //

            if (xc_state)
                vector_audio::style::button_green();

            if (ImGui::Button(std::string("XC##").append(el.callsign).c_str(),
                    quarter_size)
                && shared::datafile::facility > 0) {
                if (freq_active) {
                    mClient_->SetXc(el.freq, !xc_state);
                } else {
                    mClient_->AddFrequency(el.freq, el.callsign);
                    mClient_->UseTransceiversFromStation(el.callsign, el.freq);
                    mClient_->SetTx(el.freq, true);
                    mClient_->SetRx(el.freq, true);
                    mClient_->SetXc(el.freq, true);
                }
            }

            if (xc_state)
                vector_audio::style::button_reset_colour();

            ImGui::SameLine(0, 0.01);

            //
            // Speaker device
            //

            if (is_on_speaker)
                vector_audio::style::button_green();

            std::string transceiver_count = std::to_string(std::min(el.transceivers, 999));
            if(transceiver_count.size() < 3)
                transceiver_count.insert(0, 3 - transceiver_count.size(), ' ');
            
            std::string speaker_string = el.transceivers == -1 ? "   " : transceiver_count;
            speaker_string.append("\nSPK##");
            speaker_string.append(el.callsign);
            if (ImGui::Button(speaker_string.c_str(), quarter_size)) {
                if (freq_active)
                    mClient_->SetOnHeadset(el.freq, is_on_speaker);
            }

            if (is_on_speaker)
                vector_audio::style::button_reset_colour();

            ImGui::SameLine(0, 0.01);

            //
            // TX
            //

            if (tx_state)
                tx_active ? vector_audio::style::button_yellow()
                          : vector_audio::style::button_green();

            if (ImGui::Button(std::string("TX##").append(el.callsign).c_str(),
                    half_size)
                && shared::datafile::facility > 0) {
                if (freq_active) {
                    mClient_->SetTx(el.freq, !tx_state);
                } else {
                    mClient_->AddFrequency(el.freq, el.callsign);
                    mClient_->UseTransceiversFromStation(el.callsign, el.freq);
                    mClient_->SetTx(el.freq, true);
                    mClient_->SetRx(el.freq, true);
                }
            }

            if (tx_state)
                vector_audio::style::button_reset_colour();

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

    ImGui::PushItemWidth(-1.0);
    ImGui::Text("Add station");
    ImGui::InputText("Callsign##Auto", &shared::station_auto_add_callsign);
    ImGui::PopItemWidth();

    style::push_disabled_on(!mClient_->IsVoiceConnected());
    if (ImGui::Button("Add", ImVec2(-FLT_MIN, 0.0))) {
        if (mClient_->IsVoiceConnected()) {
            mClient_->GetStation(shared::station_auto_add_callsign);
            mClient_->FetchStationVccs(shared::station_auto_add_callsign);
            shared::station_auto_add_callsign = "";
        }
    }
    style::pop_disabled_on(!mClient_->IsVoiceConnected());

    ImGui::NewLine();

    // Gain control
    
    ImGui::PushItemWidth(-1.0);
    ImGui::Text("Radio Gain");
    style::push_disabled_on(!mClient_->IsVoiceConnected());
    if (ImGui::SliderInt("##Radio Gain", &shared::RadioGain, 0, 200, "%.3i %%")) {
        if (mClient_->IsVoiceConnected())
            mClient_->SetRadiosGain(shared::RadioGain/100.0F);
    }
    ImGui::PopItemWidth();
    style::pop_disabled_on(!mClient_->IsVoiceConnected());

    ImGui::NewLine();

    ImGui::PushItemWidth(-1.0);
    ImGui::Text("ATIS Status");
    ImGui::PopItemWidth();

    style::push_disabled_on(!(mClient_->IsVoiceConnected() && shared::datafile::atis_callsign.empty()));

    bool atis_playing_back = mClient_->IsAtisPlayingBack();
    std::string bttn_atis_playback = atis_playing_back ? "Broadcast active" : "Broadcast stopped";
    if (atis_playing_back)
        vector_audio::style::button_green();

    ImGui::Button(bttn_atis_playback.c_str(), ImVec2(-FLT_MIN, 0.0));

    // mClient->StartAtisPlayback(shared::datafile::atis_callsign,
    // shared::datafile::atis_frequency); mClient->StopAtisPlayback();

    if (atis_playing_back)
        vector_audio::style::button_reset_colour();

    bool listening_in_atis = mClient_->IsAtisListening();
    if (listening_in_atis)
        vector_audio::style::button_yellow();

    if (ImGui::Button("Listen in", ImVec2(-FLT_MIN, 0.0))) {
        if (mClient_->IsVoiceConnected() && !shared::datafile::atis_callsign.empty() && atis_playing_back) {
            mClient_->SetAtisListening(!listening_in_atis);
        }
    }

    if (listening_in_atis)
        vector_audio::style::button_reset_colour();

    style::pop_disabled_on(!(mClient_->IsVoiceConnected() && shared::datafile::atis_callsign.empty()));

    ImGui::NewLine();

    std::string rx_list = "Last RX: ";
    rx_list.append(
        received_callsigns.empty()
            ? ""
            : std::accumulate(++received_callsigns.begin(),
                received_callsigns.end(), *received_callsigns.begin(),
                [](auto& a, auto& b) { return a + ", " + b; }));
    ImGui::PushItemWidth(-1.0);
    ImGui::TextWrapped("%s", rx_list.c_str());
    ImGui::PopItemWidth();

    ImGui::NewLine();

    ImGui::TextUnformatted(vector_audio::shared::client_name.c_str());

    TextURL("Licenses",
        vector_audio::configuration::get_resource_folder() + "LICENSE.txt");

    ImGui::EndGroup();

    if (showErrorModal_) {
        ImGui::OpenPopup("Error");
        showErrorModal_ = false;
    }

    // Clear out the old API data every 500ms
    auto current_time = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - shared::currentlyTransmittingApiTimer)
            .count()
        >= 300) {
        shared::currentlyTransmittingApiData = "";

        shared::currentlyTransmittingApiData.append(
            live_received_callsigns.empty()
                ? ""
                : std::accumulate(++live_received_callsigns.begin(),
                    live_received_callsigns.end(),
                    *live_received_callsigns.begin(),
                    [](auto& a, auto& b) { return a + "," + b; }));
        shared::currentlyTransmittingApiTimer = current_time;
    }

    ImGui::End();
}

void App::errorModal(std::string message)
{
    this->showErrorModal_ = true;
    lastErrorModalMessage_ = std::move(message);
}

bool App::frequencyExists(int freq)
{
    return std::find_if(shared::FetchedStations.begin(), shared::FetchedStations.end(),
               [&freq](const auto& obj) { return obj.freq == freq; })
        != shared::FetchedStations.end();
}
} // namespace vector_audio::application