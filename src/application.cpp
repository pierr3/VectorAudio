#include "application.h"

namespace vector_audio::application {
using util::TextURL;

App::App()
    : pDataHandler(std::make_unique<vatsim::DataHandler>())
{
    try {
        afv_native::api::setLogger(
            [this](auto&& subsystem, auto&& file, auto&& line, auto&& lineOut) {
                spdlog::info("[afv_native] [{}@{}] {} {}", file, line,
                    subsystem, lineOut);
            });

        pClient = new afv_native::api::atcClient(shared::kClientName,
            vector_audio::Configuration::get_resource_folder().string());

        // Fetch all available devices on start
        vector_audio::shared::availableAudioAPI = pClient->GetAudioApis();
        vector_audio::shared::availableInputDevices
            = pClient->GetAudioInputDevices(vector_audio::shared::mAudioApi);
        vector_audio::shared::availableOutputDevices
            = pClient->GetAudioOutputDevices(vector_audio::shared::mAudioApi);
        spdlog::debug("Created afv_native client.");
    } catch (std::exception& ex) {
        spdlog::critical(
            "Could not create AFV client interface: {}", ex.what());
        return;
    }

    // Load all from config
    try {
        using cfg = vector_audio::Configuration;

        vector_audio::shared::mOutputEffects
            = toml::find_or<bool>(cfg::mConfig, "audio", "vhf_effects", true);
        vector_audio::shared::mInputFilter
            = toml::find_or<bool>(cfg::mConfig, "audio", "input_filters", true);

        vector_audio::shared::vatsimCid
            = toml::find_or<int>(cfg::mConfig, "user", "vatsim_id", 999999);
        vector_audio::shared::vatsimPassword = toml::find_or<std::string>(
            cfg::mConfig, "user", "vatsimPassword", std::string("password"));

        vector_audio::shared::keepWindowOnTop = toml::find_or<bool>(
            cfg::mConfig, "user", "keepWindowOnTop", false);

        vector_audio::shared::ptt = static_cast<sf::Keyboard::Scancode>(
            toml::find_or<int>(cfg::mConfig, "user", "ptt",
                static_cast<int>(sf::Keyboard::Scan::Unknown)));

        vector_audio::shared::joyStickId = static_cast<int>(
            toml::find_or<int>(cfg::mConfig, "user", "joyStickId", -1));
        vector_audio::shared::joyStickPtt = static_cast<int>(
            toml::find_or<int>(cfg::mConfig, "user", "joyStickPtt", -1));

        auto audioProviders = pClient->GetAudioApis();
        vector_audio::shared::configAudioApi = toml::find_or<std::string>(
            cfg::mConfig, "audio", "api", std::string("Default API"));
        for (const auto& driver : audioProviders) {
            if (driver.second == vector_audio::shared::configAudioApi)
                vector_audio::shared::mAudioApi = driver.first;
        }

        vector_audio::shared::configInputDeviceName
            = toml::find_or<std::string>(
                cfg::mConfig, "audio", "input_device", std::string(""));
        vector_audio::shared::configOutputDeviceName
            = toml::find_or<std::string>(
                cfg::mConfig, "audio", "output_device", std::string(""));
        vector_audio::shared::configSpeakerDeviceName
            = toml::find_or<std::string>(
                cfg::mConfig, "audio", "speaker_device", std::string(""));
        vector_audio::shared::headsetOutputChannel
            = toml::find_or<int>(cfg::mConfig, "audio", "headset_channel", 0);

        vector_audio::shared::hardware = static_cast<afv_native::HardwareType>(
            toml::find_or<int>(cfg::mConfig, "audio", "hardware_type", 0));

        vector_audio::shared::apiServerPort
            = toml::find_or<int>(cfg::mConfig, "general", "api_port", 49080);
    } catch (toml::exception& exc) {
        spdlog::error(
            "Failed to parse available configuration: {}", exc.what());
    }

    // Bind the callbacks from the client
    // std::bind(&App::_eventCallback, this, std::placeholders::_1,
    // std::placeholders::_2, std::placeholders::_3)
    pClient->RaiseClientEvent(
        [this](auto&& event_type, auto&& data_one, auto&& data_two) {
            eventCallback(std::forward<decltype(event_type)>(event_type),
                std::forward<decltype(data_one)>(data_one),
                std::forward<decltype(data_two)>(data_two));
        });

    // Start the API timer
    shared::currentlyTransmittingApiTimer
        = std::chrono::high_resolution_clock::now();

    // Start the SDK server
    buildSDKServer();

    // Load the airport database async
    std::thread(&vector_audio::application::App::loadAirportsDatabaseAsync)
        .detach();

    auto soundPath = Configuration::get_resource_folder()
        / std::filesystem::path("disconnect.wav");

    if (!pDisconnectWarningSoundbuffer.loadFromFile(soundPath.string())) {
        disconnectWarningSoundAvailable = false;
        spdlog::error(
            "Could not load warning sound file, disconnection will be silent");
    }

    pSoundPlayer.setBuffer(pDisconnectWarningSoundbuffer);
}

App::~App() { delete pClient; }

void App::loadAirportsDatabaseAsync()
{
    // if we cannot load this database, it's not that important, we will just
    // log it.

    if (!std::filesystem::exists(
            vector_audio::Configuration::mAirportsDBFilePath)) {
        spdlog::warn("Could not find airport database json file");
        return;
    }

    try {
        // We do performance analysis here
        auto t1 = std::chrono::high_resolution_clock::now();
        std::ifstream f(vector_audio::Configuration::mAirportsDBFilePath);
        nlohmann::json data = nlohmann::json::parse(f);

        // Loop through all the icaos
        for (const auto& obj : data.items()) {
            ns::Airport ar;
            obj.value().at("icao").get_to(ar.icao);
            obj.value().at("elevation").get_to(ar.elevation);
            obj.value().at("lat").get_to(ar.lat);
            obj.value().at("lon").get_to(ar.lon);

            // Assumption: The user will not have time to connect by the time
            // this is loaded, hence should be fine re concurrency
            ns::Airport::mAll.insert(std::make_pair(obj.key(), ar));
        }

        auto t2 = std::chrono::high_resolution_clock::now();
        spdlog::info("Loaded {} airports in {}", ns::Airport::mAll.size(),
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1));
    } catch (nlohmann::json::exception& ex) {
        spdlog::warn("Could parse airport database: {}", ex.what());
        return;
    }
}

void App::buildSDKServer()
{
    try {
        pSDKServer = restinio::run_async<>(restinio::own_io_context(),
            restinio::server_settings_t<> {}
                .port(vector_audio::shared::apiServerPort)
                .address("0.0.0.0")
                .request_handler([&](auto req) {
                    if (restinio::http_method_get() == req->header().method()
                        && req->header().request_target() == "/transmitting") {

                        const std::lock_guard<std::mutex> lock(
                            vector_audio::shared::transmittingMutex);
                        return req->create_response()
                            .set_body(vector_audio::shared::
                                    currentlyTransmittingApiData)
                            .done();
                    }
                    if (restinio::http_method_get() == req->header().method()
                        && req->header().request_target() == "/rx") {
                        std::vector<ns::Station> bar;

                        // copy only positive numbers:
                        std::copy_if(shared::fetchedStations.begin(),
                            shared::fetchedStations.end(),
                            std::back_inserter(bar),
                            [this](const ns::Station& s) {
                                if (!pClient->IsVoiceConnected())
                                    return false;
                                return pClient->GetRxState(s.getFrequencyHz());
                            });

                        std::string out;
                        if (!bar.empty()) {
                            for (auto& f : bar) {
                                out += f.getCallsign() + ":"
                                    + f.getHumanFrequency() + ",";
                            }
                        }

                        if (out.back() == ',') {
                            out.pop_back();
                        }

                        return req->create_response().set_body(out).done();
                    }
                    if (restinio::http_method_get() == req->header().method()
                        && req->header().request_target() == "/tx") {
                        std::vector<ns::Station> bar;

                        // copy only positive numbers:
                        std::copy_if(shared::fetchedStations.begin(),
                            shared::fetchedStations.end(),
                            std::back_inserter(bar),
                            [this](const ns::Station& s) {
                                if (!pClient->IsVoiceConnected())
                                    return false;
                                return pClient->GetTxState(s.getFrequencyHz());
                            });

                        std::string out;
                        if (!bar.empty()) {
                            for (auto& f : bar) {
                                out += f.getCallsign() + ":"
                                    + f.getHumanFrequency() + ",";
                            }
                        }

                        if (out.back() == ',') {
                            out.pop_back();
                        }

                        return req->create_response().set_body(out).done();
                    }

                    return req->create_response()
                        .set_body(vector_audio::shared::kClientName)
                        .done();
                }),
            16U);
    } catch (std::exception& ex) {
        spdlog::error("Failed to created SDK http server, is the port in use?");
        spdlog::error("%{}", ex.what());
    }
}

void App::eventCallback(
    afv_native::ClientEventType evt, void* data, void* data2)
{
    if (evt == afv_native::ClientEventType::VccsReceived) {
        if (data != nullptr && data2 != nullptr) {
            // We got new VCCS stations, we can add them to our list and start
            // getting their transceivers
            std::map<std::string, unsigned int> stations
                = *reinterpret_cast<std::map<std::string, unsigned int>*>(
                    data2);

            if (pClient->IsVoiceConnected()) {
                for (auto s : stations) {
                    s.second = util::cleanUpFrequency(s.second);
                    ns::Station el = ns::Station::build(s.first, s.second);

                    if (!frequencyExists(el.getFrequencyHz()))
                        shared::fetchedStations.push_back(el);
                }
            }
        }
    }

    if (evt == afv_native::ClientEventType::StationTransceiversUpdated) {
        if (data != nullptr) {
            // We just refresh the transceiver count in our display
            std::string station = *reinterpret_cast<std::string*>(data);
            auto it = std::find_if(shared::fetchedStations.begin(),
                shared::fetchedStations.end(), [station](const auto& fs) {
                    return fs.getCallsign() == station;
                });
            if (it != shared::fetchedStations.end())
                it->setTransceiverCount(
                    pClient->GetTransceiverCountForStation(station));
        }
    }

    if (evt == afv_native::ClientEventType::APIServerError) {
        // We got an error from the API server, we can display this to the user
        if (data != nullptr) {
            afv_native::afv::APISessionError err
                = *reinterpret_cast<afv_native::afv::APISessionError*>(data);

            if (err == afv_native::afv::APISessionError::BadPassword
                || err
                    == afv_native::afv::APISessionError::RejectedCredentials) {
                errorModal("Could not login to VATSIM.\nInvalid "
                           "Credentials.\nCheck your password/cid!");

                spdlog::error("Got invalid credential errors from AFV API: "
                              "HTTP 403 or 401");
            }

            if (err == afv_native::afv::APISessionError::ConnectionError) {
                errorModal("Could not login to VATSIM.\nConnection "
                           "Error.\nCheck your internet connection.");

                spdlog::error("Got connection error from AFV API: local socket "
                              "or curl error");
                disconnectAndCleanup();
                playErrorSound();
            }

            if (err
                == afv_native::afv::APISessionError::
                    BadRequestOrClientIncompatible) {
                errorModal("Could not login to VATSIM.\n Bad Request or Client "
                           "Incompatible.");

                spdlog::error("Got connection error from AFV API: HTTP 400 - "
                              "Bad Request or Client Incompatible");
                disconnectAndCleanup();
                playErrorSound();
            }

            if (err == afv_native::afv::APISessionError::InvalidAuthToken) {
                errorModal("Could not login to VATSIM.\n Invalid Auth Token.");

                spdlog::error("Got connection error from AFV API: Invalid Auth "
                              "Token Local Parse Error.");
                disconnectAndCleanup();
                playErrorSound();
            }

            if (err
                == afv_native::afv::APISessionError::
                    AuthTokenExpiryTimeInPast) {
                errorModal("Could not login to VATSIM.\n Auth Token has "
                           "expired.\n Check your system clock.");

                spdlog::error("Got connection error from AFV API: Auth Token "
                              "Expiry in the past");
                disconnectAndCleanup();
                playErrorSound();
            }

            if (err == afv_native::afv::APISessionError::OtherRequestError) {
                errorModal("Could not login to VATSIM.\n Unknown Error.");

                spdlog::error(
                    "Got connection error from AFV API: Unknown Error");

                disconnectAndCleanup();
                playErrorSound();
            }
        }
    }

    if (evt == afv_native::ClientEventType::AudioError) {
        errorModal("Error starting audio devices.\nPlease check "
                   "your log file for details.\nCheck your audio config!");
        disconnectAndCleanup();
    }

    if (evt == afv_native::ClientEventType::VoiceServerDisconnected) {

        if (!pManuallyDisconnected) {
            playErrorSound();
        }

        pManuallyDisconnected = false;
        // disconnectAndCleanup();
    }

    if (evt == afv_native::ClientEventType::VoiceServerError) {
        int errCode = *reinterpret_cast<int*>(data);
        errorModal("Voice server returned error " + std::to_string(errCode)
            + ", please check the log file.");
        disconnectAndCleanup();
        playErrorSound();
    }

    if (evt == afv_native::ClientEventType::VoiceServerChannelError) {
        int errCode = *reinterpret_cast<int*>(data);
        errorModal("Voice server returned channel error "
            + std::to_string(errCode) + ", please check the log file.");
        disconnectAndCleanup();
        playErrorSound();
    }

    if (evt == afv_native::ClientEventType::AudioDeviceStoppedError) {
        errorModal("The audio device " + *reinterpret_cast<std::string*>(data)
            + " has stopped working"
              ", check if it is still physically connected.");
        disconnectAndCleanup();
        playErrorSound();
    }

    if (evt == afv_native::ClientEventType::StationDataReceived) {
        if (data != nullptr && data2 != nullptr) {
            // We just refresh the transceiver count in our display
            bool found = *reinterpret_cast<bool*>(data);
            if (found) {
                auto station
                    = *reinterpret_cast<std::pair<std::string, unsigned int>*>(
                        data2);

                station.second = util::cleanUpFrequency(station.second);

                ns::Station el
                    = ns::Station::build(station.first, station.second);

                if (!frequencyExists(el.getFrequencyHz()))
                    shared::fetchedStations.push_back(el);
            } else {
                errorModal("Could not find station in database.");
                spdlog::warn(
                    "Station not found in AFV database through search");
            }
        }
    }
}

// Main loop
void App::render_frame()
{
    // AFV stuff
    if (pClient) {
        vector_audio::shared::mPeak = static_cast<float>(pClient->GetInputPeak());
        vector_audio::shared::mVu = static_cast<float>(pClient->GetInputVu());

        // Set the Ptt if required, input based on event
        if (pClient->IsVoiceConnected()
            && (shared::ptt != sf::Keyboard::Scan::Unknown
                || shared::joyStickId != -1)) {
            if (shared::isPttOpen) {
                if (shared::joyStickId != -1) {
                    if (!sf::Joystick::isButtonPressed(
                            shared::joyStickId, shared::joyStickPtt)) {
                        shared::isPttOpen = false;
                    }
                } else {
                    if (!sf::Keyboard::isKeyPressed(shared::ptt)) {
                        shared::isPttOpen = false;
                    }
                }

                pClient->SetPtt(shared::isPttOpen);
            } else {
                if (shared::joyStickId != -1) {
                    if (sf::Joystick::isButtonPressed(
                            shared::joyStickId, shared::joyStickPtt)) {
                        shared::isPttOpen = true;
                    }
                } else {
                    if (sf::Keyboard::isKeyPressed(shared::ptt)) {
                        shared::isPttOpen = true;
                    }
                }

                pClient->SetPtt(shared::isPttOpen);
            }
        }

        if (pClient->IsAPIConnected() && shared::fetchedStations.empty()
            && !shared::bootUpVccs) {
            // We force add the current user frequency
            shared::bootUpVccs = true;

            // We replaced double _ which may be used during frequency
            // handovers, but are not defined in database
            std::string cleanCallsign = vector_audio::util::ReplaceString(
                shared::session::callsign, "__", "_");

            ns::Station el
                = ns::Station::build(cleanCallsign, shared::session::frequency);
            if (!frequencyExists(el.getFrequencyHz()))
                shared::fetchedStations.push_back(el);

            this->pClient->AddFrequency(
                shared::session::frequency, cleanCallsign);
            pClient->SetEnableInputFilters(vector_audio::shared::mInputFilter);
            pClient->SetEnableOutputEffects(
                vector_audio::shared::mOutputEffects);
            this->pClient->UseTransceiversFromStation(
                cleanCallsign, shared::session::frequency);
            this->pClient->SetRx(shared::session::frequency, true);
            if (shared::session::facility > 0) {
                this->pClient->SetTx(shared::session::frequency, true);
                this->pClient->SetXc(shared::session::frequency, true);
            }
            this->pClient->FetchStationVccs(cleanCallsign);
            this->pClient->SetRadioGainAll(shared::radioGain / 100.0F);
        }
    }

    // The live Received callsign data
    std::vector<std::string> receivedCallsigns;
    std::vector<std::string> liveReceivedCallsigns;

    ImGui::SetNextWindowPos(ImVec2(0.0F, 0.0F));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("MainWindow", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Callsign Field
    ImGui::PushItemWidth(100.0F);
    std::string paddedCallsign = shared::session::callsign;
    std::string notConnected = "Not connected";
    if (paddedCallsign.length() < notConnected.length()) {
        paddedCallsign.insert(paddedCallsign.end(),
            notConnected.size() - paddedCallsign.size(), ' ');
    }
    ImGui::TextUnformatted(
        std::string("Callsign: ").append(paddedCallsign).c_str());
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    ImGui::SameLine();

    // Connect button logic

    if (!pClient->IsVoiceConnected() && !pClient->IsAPIConnected()) {
        bool readyToConnect = (!shared::session::isConnected
                                  && pDataHandler->isSlurperAvailable())
            || shared::session::isConnected;
        style::push_disabled_on(!readyToConnect);

        if (ImGui::Button("Connect")) {

            if (!vector_audio::shared::session::isConnected
                && pDataHandler->isSlurperAvailable()) {
                // We manually call the slurper here in case that we do not have
                // a connection yet Although this will block the whole program,
                // it is not an issue in this case As the user does not need to
                // interact with the software while we attempt A connection that
                // fails once will not be retried and will default to datafile
                // only

                vector_audio::shared::session::isConnected
                    = pDataHandler->getConnectionStatusWithSlurper();
            }

            if (vector_audio::shared::session::isConnected) {
                if (pClient->IsAudioRunning()) {
                    pClient->StopAudio();
                }
                if (pClient->IsAPIConnected()) {
                    pClient->Disconnect(); // Force a disconnect of API
                }

                pClient->SetAudioApi(findAudioAPIorDefault());
                pClient->SetAudioInputDevice(findHeadsetInputDeviceOrDefault());
                pClient->SetAudioOutputDevice(
                    findHeadsetOutputDeviceOrDefault());
                pClient->SetAudioSpeakersOutputDevice(
                    findSpeakerOutputDeviceOrDefault());
                pClient->SetHardware(vector_audio::shared::hardware);
                pClient->SetPlaybackChannelAll(
                    util::OutputChannelToAfvPlaybackChannel(
                        vector_audio::shared::headsetOutputChannel));

                if (!pDataHandler->isSlurperAvailable()) {
                    std::string clientIcao
                        = vector_audio::shared::session::callsign.substr(0,
                            vector_audio::shared::session::callsign.find('_'));
                    // We use the airport database for this
                    if (ns::Airport::mAll.find(clientIcao)
                        != ns::Airport::mAll.end()) {
                        auto clientAirport = ns::Airport::mAll.at(clientIcao);

                        // We pad the elevation by 10 meters to simulate the
                        // client being in a tower
                        pClient->SetClientPosition(clientAirport.lat,
                            clientAirport.lon, clientAirport.elevation + 33,
                            clientAirport.elevation + 33);

                        spdlog::info("Found client position in database at "
                                     "lat:{}, lon:{}, elev:{}",
                            clientAirport.lat, clientAirport.lon,
                            clientAirport.elevation);
                    } else {
                        spdlog::warn(
                            "Client position is unknown, setting default.");

                        // Default position is over Paris somewhere
                        pClient->SetClientPosition(
                            48.967860, 2.442000, 300, 300);
                    }
                } else {
                    spdlog::info(
                        "Found client position from slurper at lat:{}, lon:{}",
                        vector_audio::shared::session::latitude,
                        vector_audio::shared::session::longitude);
                    pClient->SetClientPosition(
                        vector_audio::shared::session::latitude,
                        vector_audio::shared::session::longitude, 300, 300);
                }

                pClient->SetCredentials(
                    std::to_string(vector_audio::shared::vatsimCid),
                    vector_audio::shared::vatsimPassword);
                pClient->SetCallsign(vector_audio::shared::session::callsign);
                pClient->SetRadioGainAll(shared::radioGain / 100.0F);
                if (!pClient->Connect()) {
                    spdlog::error(
                        "Failed to connect: afv_lib says API is connected.");
                };
            } else {
                errorModal("Not connected to VATSIM!");
            }
        }
        style::pop_disabled_on(!readyToConnect);
    } else {
        ImGui::PushStyleColor(
            ImGuiCol_Button, ImColor::HSV(4 / 7.0F, 0.6F, 0.6F).Value);
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered, ImColor::HSV(4 / 7.0F, 0.7F, 0.7F).Value);
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive, ImColor::HSV(4 / 7.0F, 0.8F, 0.8F).Value);

        // Auto disconnect if we need
        auto pressedDisconnect = ImGui::Button("Disconnect");
        if (pressedDisconnect || !shared::session::isConnected) {

            if (pressedDisconnect) {
                pManuallyDisconnected = true;
            }

            disconnectAndCleanup();
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine();

    // Settings modal
    style::push_disabled_on(pClient->IsAPIConnected());
    if (ImGui::Button("Settings") && !pClient->IsAPIConnected()) {
        // Update all available data
        vector_audio::shared::availableAudioAPI = pClient->GetAudioApis();
        vector_audio::shared::availableInputDevices
            = pClient->GetAudioInputDevices(vector_audio::shared::mAudioApi);
        vector_audio::shared::availableOutputDevices
            = pClient->GetAudioOutputDevices(vector_audio::shared::mAudioApi);
        ImGui::OpenPopup("Settings Panel");
    }
    style::pop_disabled_on(pClient->IsAPIConnected());

    vector_audio::modals::Settings::render(
        pClient, [this]() -> void { playErrorSound(); });

    {
        ImGui::SetNextWindowSize(ImVec2(300, -1));
        if (ImGui::BeginPopupModal("Error", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize
                    | ImGuiWindowFlags_NoResize)) {
            vector_audio::util::TextCentered(pLastErrorModalMessage);

            ImGui::NewLine();
            if (ImGui::Button("Ok", ImVec2(-FLT_MIN, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::SameLine();

    const ImVec4 red(1.0, 0.0, 0.0, 1.0);
    const ImVec4 yellow(1.0, 1.0, 0.0, 1.0);
    const ImVec4 green(0.0, 1.0, 0.0, 1.0);
    ImGui::TextColored(pClient->IsAPIConnected() ? green : red, "API");
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    ImGui::TextColored(pClient->IsVoiceConnected() ? green : red, "Voice");
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    // Status about datasource

    if (pDataHandler->isSlurperAvailable()) {
        ImGui::TextColored(green, "Slurper");
        /*if (ImGui::IsItemClicked()) {
            shared::slurper::is_unavailable = true;
        }*/
    } else if (pDataHandler->isDatafileAvailable()) {
        ImGui::TextColored(yellow, "Datafile");
    } else {
        ImGui::TextColored(red, "No VATSIM Data");
    }

    ImGui::SameLine();

    vector_audio::util::HelpMarker(
        "The data source where VectorAudio\nchecks for your VATSIM "
        "connection.\n"
        "No VATSIM Data means that VATSIM servers could not be reached at "
        "all.");

    ImGui::NewLine();

    //
    // Main area
    //

    ImGui::BeginGroup();
    ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter
        | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
        | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("stations_table", 3, flags,
            ImVec2(ImGui::GetContentRegionAvail().x * 0.8F, 0.0F))) {
        int counter = -1;
        for (auto& el : shared::fetchedStations) {
            if (counter == -1 || counter == 4) {
                counter = 1;
                ImGui::TableNextRow();
            }
            ImGui::TableSetColumnIndex(counter - 1);

            float halfHeight = ImGui::GetContentRegionAvail().x * 0.25F;
            ImVec2 halfSize
                = ImVec2(ImGui::GetContentRegionAvail().x * 0.50F, halfHeight);
            ImVec2 quarterSize
                = ImVec2(ImGui::GetContentRegionAvail().x * 0.25F, halfHeight);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.F);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.F);
            ImGui::PushStyleColor(ImGuiCol_Button, ImColor(14, 17, 22).Value);

            // Polling all data

            bool rxState = pClient->GetRxState(el.getFrequencyHz());
            bool rxActive = pClient->GetRxActive(el.getFrequencyHz());
            bool txState = pClient->GetTxState(el.getFrequencyHz());
            bool txActive = pClient->GetTxActive(el.getFrequencyHz());
            bool xcState = pClient->GetXcState(el.getFrequencyHz());
            bool isOnSpeaker = !pClient->GetOnHeadset(el.getFrequencyHz());
            bool freqActive = pClient->IsFrequencyActive(el.getFrequencyHz())
                && (rxState || txState || xcState);

            //
            // Frequency button
            //
            if (freqActive)
                vector_audio::style::button_green();
            // Disable the hover colour for this item
            ImGui::PushStyleColor(
                ImGuiCol_ButtonHovered, ImColor(14, 17, 22).Value);
            size_t callsignSize = el.getCallsign().length() / 2;
            std::string paddedFreq
                = std::string(callsignSize
                          - std::min(callsignSize,
                              el.getHumanFrequency().length() / 2),
                      ' ')
                + el.getHumanFrequency();
            std::string btnText = el.getCallsign() + "\n" + paddedFreq;
            if (ImGui::Button(btnText.c_str(), halfSize))
                ImGui::OpenPopup(el.getCallsign().c_str());
            ImGui::SameLine(0.F, 0.01F);
            ImGui::PopStyleColor();

            //
            // Frequency management popup
            //
            if (ImGui::BeginPopup(el.getCallsign().c_str())) {
                ImGui::TextUnformatted(el.getCallsign().c_str());
                ImGui::Separator();
                if (ImGui::Selectable(std::string("Force Refresh##")
                                          .append(el.getCallsign())
                                          .c_str())) {
                    pClient->FetchTransceiverInfo(el.getCallsign());
                }
                if (ImGui::Selectable(std::string("Delete##")
                                          .append(el.getCallsign())
                                          .c_str())) {
                    pClient->RemoveFrequency(el.getFrequencyHz());
                    shared::fetchedStations.erase(
                        std::remove_if(shared::fetchedStations.begin(),
                            shared::fetchedStations.end(),
                            [el](ns::Station const& p) {
                                return el.getFrequencyHz()
                                    == p.getFrequencyHz();
                            }),
                        shared::fetchedStations.end());
                }
                ImGui::EndPopup();
            }

            if (freqActive)
                vector_audio::style::button_reset_colour();

            //
            // RX Button
            //
            if (rxState) {
                // Set button colour
                rxActive ? vector_audio::style::button_yellow()
                         : vector_audio::style::button_green();

                auto receivedCld
                    = pClient->LastTransmitOnFreq(el.getFrequencyHz());
                if (!receivedCld.empty()
                    && std::find(receivedCallsigns.begin(),
                           receivedCallsigns.end(), receivedCld)
                        == receivedCallsigns.end()) {
                    receivedCallsigns.push_back(receivedCld);
                }

                // Here we filter not the last callsigns that transmitted, but
                // only the ones that are currently transmitting
                if (rxActive && !receivedCld.empty()
                    && std::find(liveReceivedCallsigns.begin(),
                           liveReceivedCallsigns.end(), receivedCld)
                        == liveReceivedCallsigns.end()) {
                    liveReceivedCallsigns.push_back(receivedCld);
                }
            }

            if (ImGui::Button(
                    std::string("RX##").append(el.getCallsign()).c_str(),
                    halfSize)) {
                if (freqActive) {
                    pClient->SetRx(el.getFrequencyHz(), !rxState);
                } else {
                    pClient->AddFrequency(
                        el.getFrequencyHz(), el.getCallsign());
                    pClient->SetEnableInputFilters(
                        vector_audio::shared::mInputFilter);
                    pClient->SetEnableOutputEffects(
                        vector_audio::shared::mOutputEffects);
                    pClient->UseTransceiversFromStation(
                        el.getCallsign(), el.getFrequencyHz());
                    pClient->SetRx(el.getFrequencyHz(), true);
                    pClient->SetRadioGainAll(shared::radioGain / 100.0F);
                }
            }

            if (rxState)
                vector_audio::style::button_reset_colour();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);

            // New line

            //
            // XC
            //

            if (xcState)
                vector_audio::style::button_green();

            if (ImGui::Button(
                    std::string("XC##").append(el.getCallsign()).c_str(),
                    quarterSize)
                && shared::session::facility > 0) {
                if (freqActive) {
                    pClient->SetXc(el.getFrequencyHz(), !xcState);
                } else {
                    pClient->AddFrequency(
                        el.getFrequencyHz(), el.getCallsign());
                    pClient->SetEnableInputFilters(
                        vector_audio::shared::mInputFilter);
                    pClient->SetEnableOutputEffects(
                        vector_audio::shared::mOutputEffects);
                    pClient->UseTransceiversFromStation(
                        el.getCallsign(), el.getFrequencyHz());
                    pClient->SetTx(el.getFrequencyHz(), true);
                    pClient->SetRx(el.getFrequencyHz(), true);
                    pClient->SetXc(el.getFrequencyHz(), true);
                    pClient->SetRadioGainAll(shared::radioGain / 100.0F);
                }
            }

            if (xcState)
                vector_audio::style::button_reset_colour();

            ImGui::SameLine(0.F, 0.01F);

            //
            // Speaker device
            //

            if (isOnSpeaker)
                vector_audio::style::button_green();

            std::string transceiverCount
                = std::to_string(std::min(el.getTransceiverCount(), 999));
            if (transceiverCount.size() < 3)
                transceiverCount.insert(0, 3 - transceiverCount.size(), ' ');

            std::string speakerString
                = el.getTransceiverCount() == -1 ? "   " : transceiverCount;
            speakerString.append("\nSPK##");
            speakerString.append(el.getCallsign());
            if (ImGui::Button(speakerString.c_str(), quarterSize)) {
                if (freqActive)
                    pClient->SetOnHeadset(el.getFrequencyHz(), isOnSpeaker);
            }

            if (isOnSpeaker)
                vector_audio::style::button_reset_colour();

            ImGui::SameLine(0.F, 0.01F);

            //
            // TX
            //

            if (txState)
                txActive ? vector_audio::style::button_yellow()
                         : vector_audio::style::button_green();

            if (ImGui::Button(
                    std::string("TX##").append(el.getCallsign()).c_str(),
                    halfSize)
                && shared::session::facility > 0) {
                if (freqActive) {
                    pClient->SetTx(el.getFrequencyHz(), !txState);
                } else {
                    pClient->AddFrequency(
                        el.getFrequencyHz(), el.getCallsign());
                    pClient->SetEnableInputFilters(
                        vector_audio::shared::mInputFilter);
                    pClient->SetEnableOutputEffects(
                        vector_audio::shared::mOutputEffects);
                    pClient->UseTransceiversFromStation(
                        el.getCallsign(), el.getFrequencyHz());
                    pClient->SetTx(el.getFrequencyHz(), true);
                    pClient->SetRx(el.getFrequencyHz(), true);
                    pClient->SetRadioGainAll(shared::radioGain / 100.0F);
                }
            }

            if (txState)
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

    style::push_disabled_on(!pClient->IsVoiceConnected());
    if (ImGui::InputText("Callsign##Auto", &shared::stationAutoAddCallsign,
            ImGuiInputTextFlags_EnterReturnsTrue
                | ImGuiInputTextFlags_AutoSelectAll
                | ImGuiInputTextFlags_CharsUppercase)
        || ImGui::Button("Add", ImVec2(-FLT_MIN, 0.0))) {
        if (pClient->IsVoiceConnected()) {
            if (!absl::StartsWith(shared::stationAutoAddCallsign, "!")
                && !absl::StartsWith(shared::stationAutoAddCallsign, "#")) {
                pClient->GetStation(shared::stationAutoAddCallsign);
                pClient->FetchStationVccs(shared::stationAutoAddCallsign);
            } else if (absl::StartsWith(shared::stationAutoAddCallsign, "!")) {
                double latitude;
                double longitude;
                shared::stationAutoAddCallsign
                    = shared::stationAutoAddCallsign.substr(1);

                if (!frequencyExists(shared::kUnicomFrequency)) {
                    if (pDataHandler->getPilotPositionWithAnything(
                            shared::stationAutoAddCallsign, latitude,
                            longitude)) {

                        ns::Station el
                            = ns::Station::build(shared::stationAutoAddCallsign,
                                shared::kUnicomFrequency);

                        shared::fetchedStations.push_back(el);
                        pClient->SetClientPosition(
                            latitude, longitude, 1000, 1000);
                        pClient->AddFrequency(shared::kUnicomFrequency,
                            shared::stationAutoAddCallsign);
                        pClient->SetRx(shared::kUnicomFrequency, true);
                        pClient->SetRadioGainAll(shared::radioGain / 100.0F);

                    } else {
                        errorModal("Could not find pilot connected under that "
                                   "callsign.");
                    }
                } else {
                    errorModal("Another UNICOM frequency is active, please "
                               "delete it first.");
                }
            } else {
                double latitude;
                double longitude;
                shared::stationAutoAddCallsign
                    = shared::stationAutoAddCallsign.substr(1);

                int frequency = 0;
                try {
                    frequency
                        = std::stoi(shared::stationAutoAddCallsign) * 1000;
                } catch (...) {
                    errorModal("Failed to parse frequency, format is #123456");
                }

                if (!frequencyExists(frequency) && frequency != 0) {
                    ns::Station el = ns::Station::build(
                        shared::stationAutoAddCallsign, frequency);

                    shared::fetchedStations.push_back(el);
                    pClient->SetClientPosition(latitude, longitude, 1000, 1000);
                    pClient->AddFrequency(frequency, "MANUAL");
                    pClient->SetRx(frequency, true);
                    pClient->SetRadioGainAll(shared::radioGain / 100.0F);
                } else {
                    errorModal("The same frequency is already active, please "
                               "delete it first.");
                }
            }

            shared::stationAutoAddCallsign = "";
        }
    }
    ImGui::PopItemWidth();
    style::pop_disabled_on(!pClient->IsVoiceConnected());

    ImGui::NewLine();

    // Gain control

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

    ImGui::NewLine();

    std::string rxList = "Last RX: ";
    rxList.append(receivedCallsigns.empty()
            ? ""
            : std::accumulate(++receivedCallsigns.begin(),
                receivedCallsigns.end(), *receivedCallsigns.begin(),
                [](auto& a, auto& b) { return a + ", " + b; }));
    ImGui::PushItemWidth(-1.0);
    ImGui::TextWrapped("%s", rxList.c_str());
    ImGui::PopItemWidth();

    ImGui::NewLine();

    // Version
    ImGui::TextUnformatted(vector_audio::shared::kClientName.c_str());

    // Licenses

    TextURL("Licenses",
        (vector_audio::Configuration::get_resource_folder() / "LICENSE.txt")
            .string());

    ImGui::EndGroup();

    if (pShowErrorModal) {
        ImGui::OpenPopup("Error");
        pShowErrorModal = false;
    }

    // Clear out the old API data every 500ms
    auto currentTime = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - shared::currentlyTransmittingApiTimer)
            .count()
        >= 300) {
        const std::lock_guard<std::mutex> lock(shared::transmittingMutex);
        shared::currentlyTransmittingApiData = "";

        shared::currentlyTransmittingApiData.append(
            liveReceivedCallsigns.empty()
                ? ""
                : std::accumulate(++liveReceivedCallsigns.begin(),
                    liveReceivedCallsigns.end(), *liveReceivedCallsigns.begin(),
                    [](auto& a, auto& b) { return a + "," + b; }));
        shared::currentlyTransmittingApiTimer = currentTime;
    }

    ImGui::End();
}

void App::errorModal(std::string message)
{
    this->pShowErrorModal = true;
    this->pLastErrorModalMessage = std::move(message);
}

bool App::frequencyExists(int freq)
{
    return std::find_if(shared::fetchedStations.begin(),
               shared::fetchedStations.end(),
               [&freq](
                   const auto& obj) { return obj.getFrequencyHz() == freq; })
        != shared::fetchedStations.end();
}
void App::disconnectAndCleanup()
{
    if (!pClient) {
        return;
    }

    pClient->Disconnect();
    pClient->StopAudio();

    for (const auto& f : shared::fetchedStations)
        pClient->RemoveFrequency(f.getFrequencyHz());

    shared::fetchedStations.clear();
    shared::bootUpVccs = false;
}

void App::playErrorSound()
{
    if (!disconnectWarningSoundAvailable) {
        return;
    }
    // Load the warning sound for disconnection
    pSoundPlayer.play();
};
} // namespace vector_audio::application