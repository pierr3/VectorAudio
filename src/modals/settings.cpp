#include "modals/settings.h"
#include "data_file_handler.h"

void vector_audio::modals::Settings::render(afv_native::api::atcClient* mClient)
{
    // Settings modal definition
    if (ImGui::BeginPopupModal("Settings Panel")) {

        ImGuiTableFlags flags = ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("settings_table", 2)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::BeginGroup();
            ImGui::Text("VATSIM Details");
            ImGui::NewLine();

            ImGui::InputInt("CID", &vector_audio::shared::vatsim_cid, 0);
            ImGui::InputText("Password", &vector_audio::shared::vatsim_password, ImGuiInputTextFlags_Password);

            ImGui::EndGroup();

            ImGui::BeginGroup();
            ImGui::NewLine();

            ImGui::TextUnformatted("Audio post-processing");
            ImGui::SameLine();
            vector_audio::util::HelpMarker("/!\\ ADVANCED SETTING /!\\ \nThis affects processing done to the audio, including\nsimmulating VHF on \
                                                                    incoming audio. In general, you\nshould at least always leave input processing on.");
            std::string filter_current_value;
            if (vector_audio::shared::mInputFilter && !vector_audio::shared::mOutputEffects) {
                filter_current_value = "Input only";
            }

            if (!vector_audio::shared::mInputFilter && vector_audio::shared::mOutputEffects) {
                filter_current_value = "Output only";
            }

            if (!vector_audio::shared::mInputFilter && !vector_audio::shared::mOutputEffects) {
                filter_current_value = "Off";
            }

            if (vector_audio::shared::mInputFilter && vector_audio::shared::mOutputEffects) {
                filter_current_value = "On";
            }

            ImGui::PushItemWidth(-1.0F);
            if (ImGui::BeginCombo("##Audio filters", filter_current_value.c_str())) {

                if (ImGui::Selectable("On", vector_audio::shared::mInputFilter && vector_audio::shared::mOutputEffects)) {
                    if (mClient) {
                        vector_audio::shared::mInputFilter = true;
                        vector_audio::shared::mOutputEffects = true;
                        mClient->SetEnableInputFilters(vector_audio::shared::mInputFilter);
                        mClient->SetEnableOutputEffects(vector_audio::shared::mOutputEffects);
                    }
                }

                if (ImGui::Selectable("Input only", vector_audio::shared::mInputFilter && !vector_audio::shared::mOutputEffects)) {
                    if (mClient) {
                        vector_audio::shared::mInputFilter = true;
                        vector_audio::shared::mOutputEffects = false;
                        mClient->SetEnableInputFilters(vector_audio::shared::mInputFilter);
                        mClient->SetEnableOutputEffects(vector_audio::shared::mOutputEffects);
                    }
                }

                if (ImGui::Selectable("Output only", !vector_audio::shared::mInputFilter && vector_audio::shared::mOutputEffects)) {
                    if (mClient) {
                        vector_audio::shared::mInputFilter = false;
                        vector_audio::shared::mOutputEffects = true;
                        mClient->SetEnableInputFilters(vector_audio::shared::mInputFilter);
                        mClient->SetEnableOutputEffects(vector_audio::shared::mOutputEffects);
                    }
                }

                if (ImGui::Selectable("Off", !vector_audio::shared::mInputFilter && !vector_audio::shared::mOutputEffects)) {
                    if (mClient) {
                        vector_audio::shared::mInputFilter = false;
                        vector_audio::shared::mOutputEffects = false;
                        mClient->SetEnableInputFilters(vector_audio::shared::mInputFilter);
                        mClient->SetEnableOutputEffects(vector_audio::shared::mOutputEffects);
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::PopItemWidth();
            ImGui::EndGroup();

            ImGui::TextUnformatted("Radio Hardware");
            ImGui::SameLine();
            vector_audio::util::HelpMarker("This setting simulates how incoming audio sounds on\ndifferent real world radio hardware.");
            ImGui::PushItemWidth(-1.0F);
            if (ImGui::BeginCombo("##Radio Hardware", vector_audio::util::getHardwareName(vector_audio::shared::hardware).c_str())) {

                if (ImGui::Selectable(vector_audio::util::getHardwareName(afv_native::HardwareType::Schmid_ED_137B).c_str(),
                        vector_audio::shared::hardware == afv_native::HardwareType::Schmid_ED_137B)) {
                    vector_audio::shared::hardware = afv_native::HardwareType::Schmid_ED_137B;
                }

                if (ImGui::Selectable(vector_audio::util::getHardwareName(afv_native::HardwareType::Garex_220).c_str(),
                        vector_audio::shared::hardware == afv_native::HardwareType::Garex_220)) {
                    vector_audio::shared::hardware = afv_native::HardwareType::Garex_220;
                }

                if (ImGui::Selectable(vector_audio::util::getHardwareName(afv_native::HardwareType::Rockwell_Collins_2100).c_str(),
                        vector_audio::shared::hardware == afv_native::HardwareType::Rockwell_Collins_2100)) {
                    vector_audio::shared::hardware = afv_native::HardwareType::Rockwell_Collins_2100;
                }

                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            ImGui::NewLine();

            ImGui::TextUnformatted("Push to talk key: ");
            std::string ptt_key_name;
            if (shared::ptt == sf::Keyboard::Scan::Unknown && shared::joyStickId == -1) {
                ptt_key_name = "Not set";
            } else if (shared::ptt != -1) {
                ptt_key_name = "Key: " + sf::Keyboard::getDescription(shared::ptt);
            } else if (shared::joyStickId != -1) {
                ptt_key_name = fmt::format("Joystick {} Button {}", shared::joyStickId, shared::joyStickPtt);
            }

            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::Button(ptt_key_name.c_str(), ImVec2(-1.0F, 0.0F));
            ImGui::PopItemFlag();

            vector_audio::style::button_blue();
            if (shared::capture_ptt_flag) {
                if (ImGui::Button("Capturing key...", ImVec2(-1.0F, 0.0F))) {
                    shared::capture_ptt_flag = false;
                }
            } else {
                if (ImGui::Button("Select New Key", ImVec2(-1.0F, 0.0F))) {
                    shared::capture_ptt_flag = true;
                }
            }
            vector_audio::style::button_reset_colour();

            ImGui::NewLine();

            ImGui::Checkbox("Keep Window On Top", &vector_audio::shared::keepWindowOnTop);
            ImGui::SameLine();
            vector_audio::util::HelpMarker("Enable this option to make the VectorAudio\nwindow stay on top of other windows.");

            ImGui::TableNextColumn();

            ImGui::Text("Audio configuration");
            ImGui::NewLine();

            ImGui::TextUnformatted("Sound API");
            ImGui::SameLine();
            vector_audio::util::HelpMarker("/!\\ ADVANCED SETTING /!\\ \nThis is the system audio api that is\nused, leaving it \
                                                                    to default should work fine.\nThis might be useful if you have multiple sound cards.");

            ImGui::PushItemWidth(-1.0F);
            if (ImGui::BeginCombo("##Sound API", vector_audio::shared::configAudioApi.c_str())) {
                // Listing the default setting
                if (ImGui::Selectable("Default", vector_audio::shared::mAudioApi == -1)) {
                    vector_audio::shared::mAudioApi = -1;
                    if (mClient) {
                        // set the Audio API and update the available inputs and outputs
                        mClient->SetAudioApi(vector_audio::shared::mAudioApi);
                        vector_audio::shared::availableInputDevices = mClient->GetAudioInputDevices(vector_audio::shared::mAudioApi);
                        vector_audio::shared::availableOutputDevices = mClient->GetAudioOutputDevices(vector_audio::shared::mAudioApi);
                    }
                    vector_audio::shared::configAudioApi = "Default";
                    vector_audio::Configuration::config_["audio"]["api"] = -1;
                }

                // Listing all the devices
                for (const auto& item : vector_audio::shared::availableAudioAPI) {
                    if (ImGui::Selectable(item.second.c_str(), vector_audio::shared::mAudioApi == item.first)) {
                        vector_audio::shared::mAudioApi = item.first;
                        if (mClient) {
                            // set the Audio API and update the available inputs and outputs
                            mClient->SetAudioApi(vector_audio::shared::mAudioApi);
                            vector_audio::shared::availableInputDevices = mClient->GetAudioInputDevices(vector_audio::shared::mAudioApi);
                            vector_audio::shared::availableOutputDevices = mClient->GetAudioOutputDevices(vector_audio::shared::mAudioApi);
                        }
                        vector_audio::shared::configAudioApi = item.second;
                        vector_audio::Configuration::config_["audio"]["api"] = item.second;
                    }
                }
                ImGui::EndCombo();
                ImGui::PopItemWidth();
            }

            ImGui::TextUnformatted("Input Device");
            ImGui::SameLine();
            vector_audio::util::HelpMarker("Select which microphone to use.");

            ImGui::PushItemWidth(-1.0F);
            if (ImGui::BeginCombo("##Input Device", vector_audio::shared::configInputDeviceName.c_str())) {

                auto m_audio_drivers = vector_audio::shared::availableInputDevices;
                for (const auto& driver : m_audio_drivers) {
                    if (ImGui::Selectable(driver.c_str(), vector_audio::shared::configInputDeviceName == driver)) {
                        vector_audio::shared::configInputDeviceName = driver;
                        vector_audio::Configuration::config_["audio"]["input_device"] = vector_audio::shared::configInputDeviceName;
                    }
                }

                ImGui::EndCombo();
                ImGui::PopItemWidth();
            }

            ImGui::TextUnformatted("Headset Device");
            ImGui::SameLine();
            vector_audio::util::HelpMarker("Select your headset device. This is the primary output device.");

            ImGui::PushItemWidth(-1.0F);
            if (ImGui::BeginCombo("##Headset Device", vector_audio::shared::configOutputDeviceName.c_str())) {

                auto m_audio_drivers = vector_audio::shared::availableOutputDevices;
                for (const auto& driver : m_audio_drivers) {
                    if (ImGui::Selectable(driver.c_str(), vector_audio::shared::configOutputDeviceName == driver)) {
                        vector_audio::shared::configOutputDeviceName = driver;
                        vector_audio::Configuration::config_["audio"]["output_device"] = vector_audio::shared::configOutputDeviceName;
                    }
                }

                ImGui::EndCombo();
                ImGui::PopItemWidth();
            }

            ImGui::TextUnformatted("Headset Playback Channel");
            ImGui::SameLine();
            vector_audio::util::HelpMarker("Optional: You can choose whether to play the\nsound in your right, left, or both ears.");

            std::string channels_display[] = { "Left + Right", "Left", "Right" };

            ImGui::PushItemWidth(-1.0F);
            if (ImGui::BeginCombo("##Channel Setup", channels_display[vector_audio::shared::headsetOutputChannel].c_str())) {

                if (ImGui::Selectable(channels_display[0].c_str(), vector_audio::shared::headsetOutputChannel == 0)) {
                    vector_audio::shared::headsetOutputChannel = 0;
                }

                if (ImGui::Selectable(channels_display[1].c_str(), vector_audio::shared::headsetOutputChannel == 1)) {
                    vector_audio::shared::headsetOutputChannel = 1;
                }

                if (ImGui::Selectable(channels_display[2].c_str(), vector_audio::shared::headsetOutputChannel == 2)) {
                    vector_audio::shared::headsetOutputChannel = 2;
                }

                ImGui::EndCombo();
                ImGui::PopItemWidth();
            }

            ImGui::TextUnformatted("Speaker Device");
            ImGui::SameLine();
            vector_audio::util::HelpMarker("Optional: Select an secondary output device which will\nbe used for any frequencies that you activate SPK for.");

            ImGui::PushItemWidth(-1.0F);
            if (ImGui::BeginCombo("##Speaker Device", vector_audio::shared::configSpeakerDeviceName.c_str())) {

                auto m_audio_drivers = vector_audio::shared::availableOutputDevices;
                for (const auto& driver : m_audio_drivers) {
                    if (ImGui::Selectable(driver.c_str(), vector_audio::shared::configSpeakerDeviceName == driver)) {
                        vector_audio::shared::configSpeakerDeviceName = driver;
                        vector_audio::Configuration::config_["audio"]["speaker_device"] = vector_audio::shared::configSpeakerDeviceName;
                    }
                }

                ImGui::EndCombo();
                ImGui::PopItemWidth();
            }

            ImGui::NewLine();

            vector_audio::style::button_purple();
            const char* audio_test_button_text = mClient->IsAudioRunning() ? "Stop Mic Test" : "Start Mic Test";
            if (ImGui::Button(audio_test_button_text, ImVec2(-1.0F, 0.0F))) {

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
            vector_audio::style::button_reset_colour();

            vector_audio::style::dualVUMeter(
                1 - (vector_audio::shared::mVu / -40.F),
                1 - (vector_audio::shared::mPeak / -40.F),
                ImVec2(-1.F, 7.F),
                ImColor(0, 200, 100),
                ImColor(219, 201, 0));

            ImGui::NewLine();
            ImGui::NewLine();

            if (ImGui::Button("Cancel", ImVec2(ImGui::GetContentRegionAvail().x * 0.30F, 0.0F))) {
                if (mClient->IsAudioRunning())
                    mClient->StopAudio();
                ImGui::CloseCurrentPopup();

                shared::capture_ptt_flag = false;
            }

            ImGui::SameLine();

            vector_audio::style::button_green();
            if (ImGui::Button("Save", ImVec2(ImGui::GetContentRegionAvail().x, 0.0F))) {
                vector_audio::Configuration::config_["user"]["vatsim_id"] = vector_audio::shared::vatsim_cid;
                vector_audio::Configuration::config_["user"]["vatsim_password"] = vector_audio::shared::vatsim_password;
                vector_audio::Configuration::config_["user"]["keepWindowOnTop"] = vector_audio::shared::keepWindowOnTop;
                vector_audio::Configuration::config_["audio"]["input_filters"] = vector_audio::shared::mInputFilter;
                vector_audio::Configuration::config_["audio"]["vhf_effects"] = vector_audio::shared::mOutputEffects;
                vector_audio::Configuration::config_["audio"]["hardware_type"] = static_cast<int>(vector_audio::shared::hardware);
                vector_audio::Configuration::config_["audio"]["headset_channel"] = vector_audio::shared::headsetOutputChannel;

                vector_audio::Configuration::write_config_async();
                if (mClient->IsAudioRunning())
                    mClient->StopAudio();
                ImGui::CloseCurrentPopup();

                shared::capture_ptt_flag = false;
            }
            vector_audio::style::button_reset_colour();

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}