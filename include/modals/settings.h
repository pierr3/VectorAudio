#pragma once
#include "atcClientWrapper.h"
#include "config.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include "shared.h"
#include "util.h"

namespace afv_unix::modals {
    class settings {
        public:
        static inline void render(afv_native::api::atcClient* mClient) {

            // Settings modal definition 
            if (ImGui::BeginPopupModal("Settings Panel"))
            {

                ImGui::BeginGroup();
                ImGui::Text("VATSIM Details");
                ImGui::PushItemWidth(200.0f);
                ImGui::InputInt("VATSIM ID", &afv_unix::shared::vatsim_cid);

                ImGui::InputText("Password", &afv_unix::shared::vatsim_password, ImGuiInputTextFlags_Password);
                ImGui::PopItemWidth();
                ImGui::EndGroup();
                
                ImGui::NewLine();

                ImGui::Text("Audio configuration");
                
                if (ImGui::BeginCombo("Sound API", afv_unix::shared::configAudioApi.c_str())) {
                    for (const auto &item: afv_unix::shared::availableAudioAPI) {
                        if (ImGui::Selectable(item.second.c_str(), afv_unix::shared::mAudioApi == item.first)) {
                            afv_unix::shared::mAudioApi = item.first;
                            if (mClient) {
                                // set the Audio API and update the available inputs and outputs
                                mClient->SetAudioApi(afv_unix::shared::mAudioApi);
                                afv_unix::shared::availableInputDevices = mClient->GetAudioInputDevices(afv_unix::shared::mAudioApi);
                                afv_unix::shared::availableOutputDevices = mClient->GetAudioOutputDevices(afv_unix::shared::mAudioApi);
                            }
                            afv_unix::shared::configAudioApi = item.second;
                            afv_unix::configuration::config["audio"]["api"] = item.second;
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("Input Device", afv_unix::shared::configInputDeviceName.c_str())) {

                    auto m_audioDrivers = afv_unix::shared::availableInputDevices;
                    for(const auto& driver : m_audioDrivers)
                    {
                        if (ImGui::Selectable(driver.c_str(), afv_unix::shared::configInputDeviceName == driver)) {
                            afv_unix::shared::configInputDeviceName = driver;
                            afv_unix::configuration::config["audio"]["input_device"] =  afv_unix::shared::configInputDeviceName;
                        }
                    }

                    ImGui::EndCombo();
                }
                
                if (ImGui::BeginCombo("Output Device", afv_unix::shared::configOutputDeviceName.c_str())) {

                    auto m_audioDrivers = afv_unix::shared::availableOutputDevices;
                    for(const auto& driver : m_audioDrivers)
                    {
                        if (ImGui::Selectable(driver.c_str(), afv_unix::shared::configOutputDeviceName == driver)) {
                            afv_unix::shared::configOutputDeviceName = driver;
                            afv_unix::configuration::config["audio"]["output_device"] =  afv_unix::shared::configOutputDeviceName;
                        }
                    }

                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("Speaker Device", afv_unix::shared::configSpeakerDeviceName.c_str())) {

                    auto m_audioDrivers = afv_unix::shared::availableOutputDevices;
                    for(const auto& driver : m_audioDrivers)
                    {
                        if (ImGui::Selectable(driver.c_str(), afv_unix::shared::configSpeakerDeviceName == driver)) {
                            afv_unix::shared::configSpeakerDeviceName = driver;
                            afv_unix::configuration::config["audio"]["speaker_device"] =  afv_unix::shared::configSpeakerDeviceName;
                        }
                    }

                    ImGui::EndCombo();
                }
                
                ImGui::NewLine();
                if (ImGui::Checkbox("Input Filter", &afv_unix::shared::mInputFilter)) {
                    if (mClient) {
                        mClient->SetEnableInputFilters(afv_unix::shared::mInputFilter);
                    } 
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("VHF Effects", &afv_unix::shared::mOutputEffects)) {
                    if (mClient) {
                        mClient->SetEnableOutputEffects(afv_unix::shared::mOutputEffects);
                    }
                }

                if (ImGui::BeginCombo("Radio Hardware", afv_unix::util::getHardwareName(afv_unix::shared::hardware).c_str())) {


                    if (ImGui::Selectable(afv_unix::util::getHardwareName(afv_native::HardwareType::Schmid_ED_137B).c_str(), 
                        afv_unix::shared::hardware == afv_native::HardwareType::Schmid_ED_137B)) 
                    {
                        afv_unix::shared::hardware = afv_native::HardwareType::Schmid_ED_137B;
                    }

                    if (ImGui::Selectable(afv_unix::util::getHardwareName(afv_native::HardwareType::Garex_220).c_str(), 
                        afv_unix::shared::hardware == afv_native::HardwareType::Garex_220)) 
                    {
                        afv_unix::shared::hardware = afv_native::HardwareType::Garex_220;
                    }

                    if (ImGui::Selectable(afv_unix::util::getHardwareName(afv_native::HardwareType::Rockwell_Collins_2100).c_str(), 
                        afv_unix::shared::hardware == afv_native::HardwareType::Rockwell_Collins_2100)) 
                    {
                        afv_unix::shared::hardware = afv_native::HardwareType::Rockwell_Collins_2100;
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
                        mClient->SetAudioApi(afv_unix::shared::mAudioApi);
                        mClient->SetAudioInputDevice(afv_unix::shared::configInputDeviceName);
                        mClient->SetAudioOutputDevice(afv_unix::shared::configOutputDeviceName);
                        mClient->SetAudioSpeakersOutputDevice(afv_unix::shared::configSpeakerDeviceName);
                        mClient->StartAudio();
                    }
                }
                ImGui::PopStyleColor(3);
                
                //float width = (ImGui::GetContentRegionAvailWidth()*0.5f)-5.0f;
                //ImGui::ProgressBar(1-(afv_unix::shared::mVu/-40.f), ImVec2(width, 0.0f));
                //ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor(0, 200, 100));
                ImGui::ProgressBar(1-(afv_unix::shared::mPeak/-40.f), ImVec2(-1.f, 7.f), "");
                ImGui::PopStyleColor();

                ImGui::NewLine();

                std::string pttText = "Push to talk key: ";
                shared::ptt == sf::Keyboard::Unknown ? pttText.append("Not set") : pttText.append(afv_unix::util::getKeyName(shared::ptt));

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
                    afv_unix::configuration::config["user"]["vatsim_id"] =  afv_unix::shared::vatsim_cid;
                    afv_unix::configuration::config["user"]["vatsim_password"] =  afv_unix::shared::vatsim_password;
                    afv_unix::configuration::config["audio"]["input_filters"] =  afv_unix::shared::mInputFilter;
                    afv_unix::configuration::config["audio"]["vhf_effects"] =  afv_unix::shared::mOutputEffects;
                    afv_unix::configuration::config["audio"]["hardware_type"] =  static_cast<int>(afv_unix::shared::hardware);

                    afv_unix::configuration::write_config_async();
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