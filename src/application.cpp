#include "application.h"


namespace afv_unix::application {

    App::App() {
        mClient = new afv_native::api::atcClient("AFV Unix Alpha");

        // Load all from config
        
        mOutputEffects = toml::find_or<bool>(afv_unix::configuration::config, "audio", "vhf_effects", true);
        mInputFilter = toml::find_or<bool>(afv_unix::configuration::config, "audio", "input_filters", true);
 
        vatsim_cid = toml::find_or<int>(afv_unix::configuration::config, "user", "vatsim_id", 999999);
        vatsim_password = toml::find_or<std::string>(afv_unix::configuration::config, "user", "vatsim_password", std::string("password"));

        auto mAudioProviders = mClient->GetAudioApis();
        configAudioApi = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "api", std::string("wrong_api"));
        for(const auto& driver : mAudioProviders)
        {
            if (driver.second == configAudioApi)
                mAudioApi = driver.first;
        }

        configInputDeviceName = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "input_device", std::string(""));
        configOutputDeviceName = toml::find_or<std::string>(afv_unix::configuration::config, "audio", "output_device", std::string(""));

    }

    App::~App() {
        delete mClient;
    }

    // Main loop
    void App::render_frame() {

        if (mClient) {
            mPeak = mClient->GetInputPeak();
            mVu = mClient->GetInputVu();
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

        if (!mClient->IsVoiceConnected()) {
            if (ImGui::Button("Connect")) {
                mClient->StopAudio();
                mClient->SetAudioApi(mAudioApi);
                mClient->SetAudioInputDevice(configInputDeviceName);
                mClient->SetAudioOutputDevice(configOutputDeviceName);
            
                mClient->SetClientPosition(48.784108, 2.2925447, 100, 100);

                //mClient->setTxRadio(0);
                //mClient->setRadioGain(0, 1.0f);
                //mClient->setRadioGain(1, 1.0f);
                mClient->SetCredentials(std::string("1259058"), vatsim_password);
                mClient->SetCallsign("ACCFR1");
                mClient->SetEnableInputFilters(mInputFilter);
                mClient->SetEnableOutputEffects(mOutputEffects);

                if (!mClient->Connect()) {
                    std::cout << "Connection failed!" << std::endl;
                };
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(4 / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(4 / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(4 / 7.0f, 0.8f, 0.8f));
            if (ImGui::Button("Disconnect")) {
                mClient->StopAudio();
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
            


        // Settings modal definition 
        if (ImGui::BeginPopupModal("Settings Panel"))
        {
            ImGui::Text("VATSIM Details");
            ImGui::InputInt("VATSIM ID", &vatsim_cid);

            ImGui::InputText("Password", &vatsim_password, ImGuiInputTextFlags_Password);

            ImGui::NewLine();

            ImGui::Text("Audio configuration");
            
            if (ImGui::BeginCombo("Sound API", configAudioApi.c_str())) {
                for (const auto &item: mClient->GetAudioApis()) {
                    if (ImGui::Selectable(item.second.c_str(), mAudioApi == item.first)) {
                        mAudioApi = item.first;
                        if (mClient)
                            mClient->SetAudioApi(mAudioApi);
                        afv_unix::configuration::config["audio"]["api"] = item.second;
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::BeginCombo("Input Device", configInputDeviceName.c_str())) {

                auto m_audioDrivers = mClient->GetAudioInputDevices(mAudioApi);
                for(const auto& driver : m_audioDrivers)
                {
                    if (ImGui::Selectable(driver.c_str(), configInputDeviceName == driver)) {
                        configInputDeviceName = driver;
                        afv_unix::configuration::config["audio"]["input_device"] =  configInputDeviceName;
                    }
                }

                ImGui::EndCombo();
            }

            if (ImGui::BeginCombo("Output Device", configOutputDeviceName.c_str())) {

                auto m_audioDrivers = mClient->GetAudioOutputDevices(mAudioApi);
                for(const auto& driver : m_audioDrivers)
                {
                    if (ImGui::Selectable(driver.c_str(), configOutputDeviceName == driver)) {
                        configOutputDeviceName = driver;
                        afv_unix::configuration::config["audio"]["output_device"] =  configOutputDeviceName;
                    }
                }

                ImGui::EndCombo();
            }
            
            ImGui::NewLine();
            if (ImGui::Checkbox("Input Filter", &mInputFilter)) {
                if (mClient) {
                    mClient->SetEnableInputFilters(mInputFilter);
                } 
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("VHF Effects", &mOutputEffects)) {
                if (mClient) {
                    mClient->SetEnableOutputEffects(mOutputEffects);
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
        ImGui::TextColored(mClient->IsAPIConnected() ? green : red, "API Server Connection");ImGui::SameLine(); ImGui::Text(" | "); ImGui::SameLine();
        ImGui::TextColored(mClient->IsVoiceConnected() ? green : red, "Voice Server Connection");

        ImGui::SliderFloat("Input VU", &mVu, -60.0f, 0.0f);
        ImGui::SliderFloat("Input Peak", &mPeak, -60.0f, 0.0f);

        ImGui::End();

    }

}