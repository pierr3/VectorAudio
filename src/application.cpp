#include "application.h"


namespace afv_unix::application {

    App::App() {
        mEventBase = event_base_new();
        mClient = std::make_shared<afv_native::Client>(mEventBase, 2, "afv-unix");
        //mClient = new afv_native::Client(mEventBase, 2,"AFV-Unix-TestClient");

        // auto m_audioDrivers = afv_native::audio::AudioDevice::getAPIs();
        // for(const auto& driver : m_audioDrivers)
        // {
        //     m_audioApi = driver.first;
        //     break;
        // }
    }

    App::~App(){
        
    }


    // Main loop
    void App::render_frame() {

        // if (mClient) {
        //     mPeak = mClient->getInputPeak();
        //     mVu = mClient->getInputVu();
        // }

        #ifdef IMGUI_HAS_VIEWPORT
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->GetWorkPos());
            ImGui::SetNextWindowSize(viewport->GetWorkSize());
            ImGui::SetNextWindowViewport(viewport->ID);
        #else 
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        #endif
        bool open = true;
        ImGui::Begin("MainWindow", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
        
        // Connect button logic
        ImGui::Button("Connect");

        ImGui::SameLine();

        // Settings modal
        if (ImGui::Button("Settings"))
            ImGui::OpenPopup("Settings Panel");


        // Modal definition 
        if (ImGui::BeginPopupModal("Settings Panel"))
        {

            ImGui::Text("VATSIM Details");
            static int i0 = 123;
            ImGui::InputInt("VATSIM ID", &i0);

            static char password[64] = "password123";
            ImGui::InputText("Password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);

            ImGui::NewLine();

            ImGui::Text("Audio configuration");
            
            if (ImGui::BeginCombo("Sound API", nameForAudioApi(mAudioApi).c_str())) {
                if (ImGui::Selectable("Default", mAudioApi == 0)) {
                    //setAudioApi(0);
                }
                for (const auto &item: mAudioProviders) {
                    if (ImGui::Selectable(item.second.c_str(), mAudioApi == item.first)) {
                        //setAudioApi(item.first);
                    }
                }
                ImGui::EndCombo();
            }

            const char *currentInputDevice = "None Selected";

            if (ImGui::BeginCombo("Input Device", currentInputDevice)) {

                auto m_audioDrivers = afv_native::audio::AudioDevice::getCompatibleInputDevicesForApi(mAudioApi);
                for(const auto& driver : m_audioDrivers)
                {
                    if (ImGui::Selectable(driver.second.name.c_str(), mInputDevice == driver.first)) {
                        mInputDevice = driver.first;
                    }
                }

                ImGui::EndCombo();
            }

            const char *currentOutputDevice = "None Selected";

            if (ImGui::BeginCombo("Output Device", currentInputDevice)) {

                auto m_audioDrivers = afv_native::audio::AudioDevice::getCompatibleOutputDevicesForApi(mAudioApi);
                for(const auto& driver : m_audioDrivers)
                {
                    if (ImGui::Selectable(driver.second.name.c_str(), mOutputDevice == driver.first)) {
                        mOutputDevice = driver.first;
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

            if (ImGui::Button("Save"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::Text("Connected as: not connected");


        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

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