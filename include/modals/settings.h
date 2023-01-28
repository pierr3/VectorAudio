#pragma once
#include "atcClientWrapper.h"
#include "config.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include "shared.h"
#include "util.h"

namespace vector_audio::modals {
    class settings {
        public:
        static inline void render(afv_native::api::atcClient* mClient) {

            // Settings modal definition 
            if (ImGui::BeginPopupModal("Settings Panel"))
            {

                ImGui::BeginGroup();
                ImGui::Text("VATSIM Details");
                ImGui::PushItemWidth(200.0f);
                ImGui::InputInt("VATSIM ID", &vector_audio::shared::vatsim_cid);

                ImGui::InputText("Password", &vector_audio::shared::vatsim_password, ImGuiInputTextFlags_Password);
                ImGui::PopItemWidth();
                ImGui::EndGroup();
                
                ImGui::NewLine();

                ImGui::Text("Audio configuration");
                
                if (ImGui::BeginCombo("Sound API", vector_audio::shared::configAudioApi.c_str())) {
                    for (const auto &item: vector_audio::shared::availableAudioAPI) {
                        if (ImGui::Selectable(item.second.c_str(), vector_audio::shared::mAudioApi == item.first)) {
                            vector_audio::shared::mAudioApi = item.first;
                            if (mClient) {
                                // set the Audio API and update the available inputs and outputs
                                mClient->SetAudioApi(vector_audio::shared::mAudioApi);
                                vector_audio::shared::availableInputDevices = mClient->GetAudioInputDevices(vector_audio::shared::mAudioApi);
                                vector_audio::shared::availableOutputDevices = mClient->GetAudioOutputDevices(vector_audio::shared::mAudioApi);
                            }
                            vector_audio::shared::configAudioApi = item.second;
                            vector_audio::configuration::config["audio"]["api"] = item.second;
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("Input Device", vector_audio::shared::configInputDeviceName.c_str())) {

                    auto m_audioDrivers = vector_audio::shared::availableInputDevices;
                    for(const auto& driver : m_audioDrivers)
                    {
                        if (ImGui::Selectable(driver.c_str(), vector_audio::shared::configInputDeviceName == driver)) {
                            vector_audio::shared::configInputDeviceName = driver;
                            vector_audio::configuration::config["audio"]["input_device"] =  vector_audio::shared::configInputDeviceName;
                        }
                    }

                    ImGui::EndCombo();
                }
                
                if (ImGui::BeginCombo("Output Device", vector_audio::shared::configOutputDeviceName.c_str())) {

                    auto m_audioDrivers = vector_audio::shared::availableOutputDevices;
                    for(const auto& driver : m_audioDrivers)
                    {
                        if (ImGui::Selectable(driver.c_str(), vector_audio::shared::configOutputDeviceName == driver)) {
                            vector_audio::shared::configOutputDeviceName = driver;
                            vector_audio::configuration::config["audio"]["output_device"] =  vector_audio::shared::configOutputDeviceName;
                        }
                    }

                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("Speaker Device", vector_audio::shared::configSpeakerDeviceName.c_str())) {

                    auto m_audioDrivers = vector_audio::shared::availableOutputDevices;
                    for(const auto& driver : m_audioDrivers)
                    {
                        if (ImGui::Selectable(driver.c_str(), vector_audio::shared::configSpeakerDeviceName == driver)) {
                            vector_audio::shared::configSpeakerDeviceName = driver;
                            vector_audio::configuration::config["audio"]["speaker_device"] =  vector_audio::shared::configSpeakerDeviceName;
                        }
                    }

                    ImGui::EndCombo();
                }
                
                ImGui::NewLine();
                if (ImGui::Checkbox("Input Filter", &vector_audio::shared::mInputFilter)) {
                    if (mClient) {
                        mClient->SetEnableInputFilters(vector_audio::shared::mInputFilter);
                    } 
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("VHF Effects", &vector_audio::shared::mOutputEffects)) {
                    if (mClient) {
                        mClient->SetEnableOutputEffects(vector_audio::shared::mOutputEffects);
                    }
                }

                if (ImGui::BeginCombo("Radio Hardware", vector_audio::util::getHardwareName(vector_audio::shared::hardware).c_str())) {


                    if (ImGui::Selectable(vector_audio::util::getHardwareName(afv_native::HardwareType::Schmid_ED_137B).c_str(), 
                        vector_audio::shared::hardware == afv_native::HardwareType::Schmid_ED_137B)) 
                    {
                        vector_audio::shared::hardware = afv_native::HardwareType::Schmid_ED_137B;
                    }

                    if (ImGui::Selectable(vector_audio::util::getHardwareName(afv_native::HardwareType::Garex_220).c_str(), 
                        vector_audio::shared::hardware == afv_native::HardwareType::Garex_220)) 
                    {
                        vector_audio::shared::hardware = afv_native::HardwareType::Garex_220;
                    }

                    if (ImGui::Selectable(vector_audio::util::getHardwareName(afv_native::HardwareType::Rockwell_Collins_2100).c_str(), 
                        vector_audio::shared::hardware == afv_native::HardwareType::Rockwell_Collins_2100)) 
                    {
                        vector_audio::shared::hardware = afv_native::HardwareType::Rockwell_Collins_2100;
                    }

                    ImGui::EndCombo();
                }


                ImGui::NewLine();

                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(5 / 7.0f, 0.6f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(5 / 7.0f, 0.7f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(5 / 7.0f, 0.8f, 0.8f));
                const char* audioTestButtonText = mClient->IsAudioRunning() ? "Stop Mic Test" : "Start Mic Test";
                if (ImGui::Button(audioTestButtonText, ImVec2(-1.0f, 0.0f))) {


                    if (mClient->IsAudioRunning())
                        mClient->StopAudio();
                    else {
                        mClient->SetAudioApi(vector_audio::shared::mAudioApi);
                        mClient->SetAudioInputDevice(vector_audio::shared::configInputDeviceName);
                        mClient->SetAudioOutputDevice(vector_audio::shared::configOutputDeviceName);
                        mClient->SetAudioSpeakersOutputDevice(vector_audio::shared::configSpeakerDeviceName);
                        mClient->StartAudio();
                    }
                }
                ImGui::PopStyleColor(3);
                
                //float width = (ImGui::GetContentRegionAvailWidth()*0.5f)-5.0f;
                //ImGui::ProgressBar(1-(vector_audio::shared::mVu/-40.f), ImVec2(width, 0.0f));
                //ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor(0, 200, 100));
                ImGui::ProgressBar(1-(vector_audio::shared::mPeak/-40.f), ImVec2(-1.f, 7.f), "");
                ImGui::PopStyleColor();

                ImGui::NewLine();

                std::string pttText = "Push to talk key: ";
                shared::ptt == sf::Keyboard::Unknown ? pttText.append("Not set") : pttText.append(vector_audio::util::getKeyName(shared::ptt));

                ImGui::TextUnformatted(pttText.c_str());
                ImGui::SameLine();
                ImGui::Checkbox("Select new key", &shared::capture_ptt_flag);

                ImGui::NewLine();

                if (ImGui::Button("Cancel")) {
                    if (mClient->IsAudioRunning())
                        mClient->StopAudio();
                    ImGui::CloseCurrentPopup();

                    shared::capture_ptt_flag = false;
                }
                    
                ImGui::SameLine();

                if (ImGui::Button("Save")) {
                    vector_audio::configuration::config["user"]["vatsim_id"] =  vector_audio::shared::vatsim_cid;
                    vector_audio::configuration::config["user"]["vatsim_password"] =  vector_audio::shared::vatsim_password;
                    vector_audio::configuration::config["audio"]["input_filters"] =  vector_audio::shared::mInputFilter;
                    vector_audio::configuration::config["audio"]["vhf_effects"] =  vector_audio::shared::mOutputEffects;
                    vector_audio::configuration::config["audio"]["hardware_type"] =  static_cast<int>(vector_audio::shared::hardware);

                    vector_audio::configuration::write_config_async();
                    if (mClient->IsAudioRunning())
                        mClient->StopAudio();
                    ImGui::CloseCurrentPopup();


                    shared::capture_ptt_flag = false;
                }
                    

                ImGui::EndPopup();
            }

        }
    };
}