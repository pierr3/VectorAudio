#include "application.h"


namespace afv_unix::application {

    App::App() {
        mEventBase = event_base_new();
        mClient = std::make_shared<afv_native::Client>(mEventBase, 1, "afv-unix-alpha");

        // Load all from config
        
        mOutputEffects = toml::find_or<bool>(afv_unix::configuration::config, "audio", "vhf_effects", true);
        mInputFilter = toml::find_or<bool>(afv_unix::configuration::config, "audio", "input_filters", true);
 
        vatsim_cid = toml::find_or<int>(afv_unix::configuration::config, "user", "vatsim_id", 999999);
        vatsim_password = toml::find_or<std::string>(afv_unix::configuration::config, "user", "vatsim_password", std::string("password"));

        mAudioProviders = afv_native::audio::AudioDevice::getAPIs();
        std::string configApi = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "api", std::string("wrong_api"));
        for(const auto& driver : mAudioProviders)
        {
            if (driver.second == configApi)
                mAudioApi = driver.first;
        }

        mInputDevices = afv_native::audio::AudioDevice::getCompatibleInputDevicesForApi(mAudioApi);
        configInputDeviceName = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "input_device", std::string(""));
        for(const auto& driver : mInputDevices)
        {
            if (driver.second.name == configInputDeviceName)
                mInputDevice = driver.first;
        }

        mOutputDevices = afv_native::audio::AudioDevice::getCompatibleOutputDevicesForApi(mAudioApi);
        configOutputDeviceName = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "output_device", std::string(""));
        for(const auto& driver : mOutputDevices)
        {
            if (driver.second.name == configOutputDeviceName)
                mOutputDevice = driver.first;
        }

        mClient->ClientEventCallback.addCallback(nullptr, [&](afv_native::ClientEventType evt, void* data)
        {
            switch(evt)
            {
            case afv_native::ClientEventType::APIServerError:
                if(data != nullptr) {
                    auto error = *reinterpret_cast<afv_native::afv::APISessionError*>(data);
                    switch(error) {
                    case afv_native::afv::APISessionError::BadPassword:
                    case afv_native::afv::APISessionError::RejectedCredentials:
                        std::cout << "APISession" << "Error connecting to voice server. Please check your VATSIM credentials and try again." << std::endl;
                        break;
                    case afv_native::afv::APISessionError::ConnectionError:
                        std::cout <<"APISession"<< "Error initiating voice server connection."<< std::endl;
                        break;
                    default:
                        break;
                    }
                }
                break;
            case afv_native::ClientEventType::VoiceServerChannelError:
                if(data != nullptr) {
                    int error = *reinterpret_cast<int*>(data);
                    std::cout <<"VoiceServer"<< "Voice server error: "<< std::endl;
                    //emit notificationPosted((int)NotificationType::Error, QString("Voice server error: %s").arg(error));
                }
                break;
            case afv_native::ClientEventType::VoiceServerError:
                if(data != nullptr) {
                    auto error = *reinterpret_cast<afv_native::afv::VoiceSessionError*>(data);
                    switch(error) {
                    case afv_native::afv::VoiceSessionError::BadResponseFromAPIServer:
                        std::cout <<"VoiceServerError"<< "Voice server error: BadResponseFromAPIServer"<< std::endl;
                        break;
                    case afv_native::afv::VoiceSessionError::Timeout:
                        std::cout <<"VoiceServerError" << "Voice server error: Timeout"<< std::endl;
                        break;
                    case afv_native::afv::VoiceSessionError::UDPChannelError:
                        std::cout <<"VoiceServerError"<< "Voice server error: UDPChannelError"<< std::endl;
                        break;
                    default:
                        break;
                    }
                }
                break;
            case afv_native::ClientEventType::VoiceServerConnected:
                std::cout <<"Connection"<< "Connected to voice server."<< std::endl;
                break;
            case afv_native::ClientEventType::VoiceServerDisconnected:
                std::cout <<"Connection"<< "Disconnected from voice server."<< std::endl;
                break;
            default:
                break;
            }
        });

        //afv_native::setLogger(nullptr);

    }

    App::~App(){
        
    }

    // Main loop
    void App::render_frame() {

        if (mClient) {
            mPeak = mClient->getInputPeak();
            mVu = mClient->getInputVu();
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
        
        // Connect button logic

        if (!mClient->isVoiceConnected()) {
            if (ImGui::Button("Connect")) {
                mClient->stopAudio();
                mClient->setAudioApi(mAudioApi);
                mClient->setAudioInputDevice(mInputDevices.at(mInputDevice).id);
                mClient->setAudioOutputDevice(mOutputDevices.at(mOutputDevice).id);
                mClient->setRadioState(0, 5000);
                mClient->setRadioState(1, 5000);

                mClient->setClientPosition(48.784108, 2.2925447, 100, 100);

                mClient->setTxRadio(0);
                mClient->setRadioGain(0, 1.0f);
                mClient->setRadioGain(1, 1.0f);
                mClient->setCredentials(std::string("1259058"), vatsim_password);
                mClient->setCallsign("ACCFR1");
                mClient->setEnableInputFilters(mInputFilter);
                mClient->setEnableOutputEffects(mOutputEffects);
                mClient->startAudio();
                if (!mClient->connect()) {
                    std::cout << "Connection failed!" << std::endl;

                };
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(4 / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(4 / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(4 / 7.0f, 0.8f, 0.8f));
            if (ImGui::Button("Disconnect")) {
                mClient->stopAudio();
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();

        // Settings modal
        if (mClient->isVoiceConnected()) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        
        if (ImGui::Button("Settings") && !mClient->isVoiceConnected())
            ImGui::OpenPopup("Settings Panel");

        if (mClient->isVoiceConnected()) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
            


        // Settings modal definition 
        if (ImGui::BeginPopupModal("Settings Panel"))
        {
            ImGui::Text("VATSIM Details");
            ImGui::InputInt("VATSIM ID", &vatsim_cid);

            ImGui::InputText("Password", &vatsim_password, ImGuiInputTextFlags_Password);

            ImGui::NewLine();

            ImGui::Text("Audio configuration");
            
            if (ImGui::BeginCombo("Sound API", nameForAudioApi(mAudioApi).c_str())) {
                for (const auto &item: mAudioProviders) {
                    if (ImGui::Selectable(item.second.c_str(), mAudioApi == item.first)) {
                        mAudioApi = item.first;
                        if (mClient)
                            mClient->setAudioApi(mAudioApi);
                        afv_unix::configuration::config["audio"]["api"] = item.second;
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::BeginCombo("Input Device", configInputDeviceName.c_str())) {

                auto m_audioDrivers = afv_native::audio::AudioDevice::getCompatibleInputDevicesForApi(mAudioApi);
                for(const auto& driver : m_audioDrivers)
                {
                    if (ImGui::Selectable(driver.second.name.c_str(), mInputDevice == driver.first)) {
                        mInputDevice = driver.first;
                        configInputDeviceName = driver.second.name;
                        afv_unix::configuration::config["audio"]["input_device"] =  driver.second.name;
                    }
                }

                ImGui::EndCombo();
            }

            if (ImGui::BeginCombo("Output Device", configOutputDeviceName.c_str())) {

                auto m_audioDrivers = afv_native::audio::AudioDevice::getCompatibleOutputDevicesForApi(mAudioApi);
                for(const auto& driver : m_audioDrivers)
                {
                    if (ImGui::Selectable(driver.second.name.c_str(), mOutputDevice == driver.first)) {
                        mOutputDevice = driver.first;
                        afv_unix::configuration::config["audio"]["output_device"] =  driver.second.name;
                        configOutputDeviceName = driver.second.name;
                    }
                }

                ImGui::EndCombo();
            }
            
            ImGui::NewLine();
            if (ImGui::Checkbox("Input Filter", &mInputFilter)) {
                if (mClient) {
                    mClient->setEnableInputFilters(mInputFilter);
                } 
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("VHF Effects", &mOutputEffects)) {
                if (mClient) {
                    mClient->setEnableOutputEffects(mOutputEffects);
                }
            }

            ImGui::NewLine();
            ImGui::NewLine();

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::SameLine();

            if (ImGui::Button("Save")) {
                afv_unix::configuration::config["user"]["vatsim_id"] =  vatsim_cid;
                afv_unix::configuration::config["user"]["vatsim_password"] =  vatsim_password;
                afv_unix::configuration::config["audio"]["input_filters"] =  mInputFilter;
                afv_unix::configuration::config["audio"]["vhf_effects"] =  mOutputEffects;


                afv_unix::configuration::write_config_async();
                ImGui::CloseCurrentPopup();
            }
                

            ImGui::EndPopup();
        }

        ImGui::SameLine();

        const ImVec4 red(1.0, 0.0, 0.0, 1.0), green(0.0, 1.0, 0.0, 1.0);
        ImGui::TextColored(mClient->isAPIConnected() ? green : red, "API Server Connection");ImGui::SameLine(); ImGui::Text(" | "); ImGui::SameLine();
        ImGui::TextColored(mClient->isVoiceConnected() ? green : red, "Voice Server Connection");

        ImGui::SliderFloat("Input VU", &mVu, -60.0f, 0.0f);
        ImGui::SliderFloat("Input Peak", &mPeak, -60.0f, 0.0f);

        ImGui::End();

    }

    std::string App::nameForAudioApi(afv_native::audio::AudioDevice::Api apiNum) {
        auto mIter = mAudioProviders.find(apiNum);
        if (mIter == mAudioProviders.end()) {
            if (apiNum == 0) return "UNSPECIFIED (default)";
            return "INVALID";
        }
        return mIter->second;
    }

}