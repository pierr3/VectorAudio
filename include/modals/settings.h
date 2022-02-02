#pragma once
#include "AFVatcClientWrapper.h"
#include "config.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include "shared.h"

namespace afv_unix::modals {
    class settings {
        public:
        static inline void render(afv_native::api::atcClient* mClient) {

            // Settings modal definition 
            if (ImGui::BeginPopupModal("Settings Panel"))
            {
                ImGui::Text("VATSIM Details");
                ImGui::InputInt("VATSIM ID", &afv_unix::shared::vatsim_cid);

                ImGui::InputText("Password", &afv_unix::shared::vatsim_password, ImGuiInputTextFlags_Password);

                ImGui::NewLine();

                ImGui::Text("Audio configuration");
                
                if (ImGui::BeginCombo("Sound API", afv_unix::shared::configAudioApi.c_str())) {
                    for (const auto &item: mClient->GetAudioApis()) {
                        if (ImGui::Selectable(item.second.c_str(), afv_unix::shared::mAudioApi == item.first)) {
                            afv_unix::shared::mAudioApi = item.first;
                            if (mClient)
                                mClient->SetAudioApi(afv_unix::shared::mAudioApi);
                            afv_unix::shared::configAudioApi = item.second;
                            afv_unix::configuration::config["audio"]["api"] = item.second;
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("Input Device", afv_unix::shared::configInputDeviceName.c_str())) {

                    auto m_audioDrivers = mClient->GetAudioInputDevices(afv_unix::shared::mAudioApi);
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

                    auto m_audioDrivers = mClient->GetAudioOutputDevices(afv_unix::shared::mAudioApi);
                    for(const auto& driver : m_audioDrivers)
                    {
                        if (ImGui::Selectable(driver.c_str(), afv_unix::shared::configOutputDeviceName == driver)) {
                            afv_unix::shared::configOutputDeviceName = driver;
                            afv_unix::configuration::config["audio"]["output_device"] =  afv_unix::shared::configOutputDeviceName;
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

                ImGui::NewLine();


                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(5 / 7.0f, 0.6f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(5 / 7.0f, 0.7f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(5 / 7.0f, 0.8f, 0.8f));
                const char* audioTestButtonText = mClient->IsAudioRunning() ? "Stop Audio Test" : "Start Audio Test";
                if (ImGui::Button(audioTestButtonText, ImVec2(ImGui::GetContentRegionAvailWidth()*0.7f, 0.0f))) {
                    if (mClient->IsAudioRunning())
                        mClient->StopAudio();
                    else
                        mClient->StartAudio();
                }
                ImGui::PopStyleColor(3);

                ImGui::ProgressBar(1-(afv_unix::shared::mVu/-60.f), ImVec2(-80.0f, 0.0f));
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::Text("Input VU");

                ImGui::ProgressBar(1-(afv_unix::shared::mPeak/-60.f), ImVec2(-80.0f, 0.0f));
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                ImGui::Text("Input Peak");

                ImGui::NewLine();
                ImGui::NewLine();

                if (ImGui::Button("Cancel")) {
                    if (mClient->IsAudioRunning())
                        mClient->StopAudio();
                    ImGui::CloseCurrentPopup();
                }
                    
                ImGui::SameLine();

                if (ImGui::Button("Save")) {
                    afv_unix::configuration::config["user"]["vatsim_id"] =  afv_unix::shared::vatsim_cid;
                    afv_unix::configuration::config["user"]["vatsim_password"] =  afv_unix::shared::vatsim_password;
                    afv_unix::configuration::config["audio"]["input_filters"] =  afv_unix::shared::mInputFilter;
                    afv_unix::configuration::config["audio"]["vhf_effects"] =  afv_unix::shared::mOutputEffects;


                    afv_unix::configuration::write_config_async();
                    if (mClient->IsAudioRunning())
                        mClient->StopAudio();
                    ImGui::CloseCurrentPopup();
                }
                    

                ImGui::EndPopup();
            }

        }
    };
}