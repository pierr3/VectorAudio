#pragma once
#include "imgui.h"
#include "shared.h"
#include <SFML/Window/Keyboard.hpp>
#include <afv-native/hardwareType.h>
#include <algorithm>
#include <string>
#include <utility>

namespace vector_audio::util {

inline static std::string getHardwareName(
    const afv_native::HardwareType hardware)
{
    switch (hardware) {
    case afv_native::HardwareType::Garex_220:
        return "Garex 220";
    case afv_native::HardwareType::Rockwell_Collins_2100:
        return "Rockwell Collins 2100";
    case afv_native::HardwareType::Schmid_ED_137B:
        return "Schmid ED-137B";
    }

    return "Unknown Hardware";
}

inline static std::string ReplaceString(
    std::string subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

inline void AddUnderLine(ImColor col_)
{
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    min.y = max.y;
    ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0F);
}

// From Imgui demo
static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)
        && ImGui::BeginTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0F);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// https://stackoverflow.com/questions/64653747/how-to-center-align-text-horizontally
inline void TextCentered(const std::string& text)
{
    float win_width = ImGui::GetWindowSize().x;
    float text_width = ImGui::CalcTextSize(text.c_str()).x;

    // calculate the indentation that centers the text on one line, relative
    // to window left, regardless of the `ImGuiStyleVar_WindowPadding` value
    float text_indentation = (win_width - text_width) * 0.5F;

    // if text is too long to be drawn on one line, `text_indentation` can
    // become too small or even negative, so we check a minimum indentation
    float min_indentation = 20.0F;
    if (text_indentation <= min_indentation) {
        text_indentation = min_indentation;
    }

    ImGui::SameLine(text_indentation);
    ImGui::PushTextWrapPos(win_width - text_indentation);
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopTextWrapPos();
}

inline int PlatformOpen(std::string cmd)
{
#ifdef SFML_SYSTEM_WINDOWS
    cmd = "explorer " + cmd;
    return std::system(cmd.c_str());
#endif

#ifdef SFML_SYSTEM_MACOS
    cmd = "open " + cmd;
    return std::system(cmd.c_str());
#endif

#ifdef SFML_SYSTEM_LINUX
    int err = std::system(std::string("gedit " + cmd)
                              .c_str()); // This assume gnome environment, we
                                         // will also cover KDE if this fails
    if (err != 0) {
        return std::system(std::string("kate " + cmd).c_str());
    }
    return err;
#endif
}

// From
// https://github.com/swift-project/pilotclient/blob/main/src/blackmisc/aviation/comsystem.cpp
// Under GPL v3 License

inline bool isValid8_33kHzChannel(int fKHz)
{
    fKHz = fKHz / 100;
    const int last_digits = static_cast<int>(fKHz) % 100;
    return fKHz % 5 == 0 && last_digits != 20 && last_digits != 45
        && last_digits != 70 && last_digits != 95;
}

inline int round8_33kHzChannel(int fKHz)
{
    fKHz = fKHz / 100;
    if (!isValid8_33kHzChannel(fKHz)) {
        const int diff = static_cast<int>(fKHz) % 5;
        int lower = fKHz - diff;
        if (!isValid8_33kHzChannel(lower)) {
            lower -= 5;
        }

        int upper = fKHz + (5 - diff);
        if (!isValid8_33kHzChannel(upper)) {
            upper += 5;
        }

        const int lower_diff = abs(fKHz - lower);
        const int upper_diff = abs(fKHz - upper);

        fKHz = lower_diff < upper_diff ? lower : upper;
        fKHz = std::clamp(fKHz, 118000, 136990);
    }
    return fKHz*100;
}

inline void TextURL(const std::string& name_, std::string URL_)
{
    ImGui::PushStyleColor(
        ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
    ImGui::TextUnformatted(name_.c_str());
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseClicked(0)) {
            util::PlatformOpen(std::move(URL_));
        }
        AddUnderLine(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
    } else {
        AddUnderLine(ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }
}

inline int roundUpToMultiplier(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

inline int cleanUpFrequency(int frequency)
{
    // Ensures that a frequency is always valid and within defined ranges and in
    // 25Khz format

    // We don't clean up an unset frequency
    if (std::abs(frequency) == shared::kObsFrequency) {
        return frequency;
    }

    if (!util::isValid8_33kHzChannel(frequency)) {
        return util::round8_33kHzChannel(frequency);
    }

    return std::clamp(frequency, 118000000, 136990000);;
}

static bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size()
        && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static bool startsWith(const std::string& str, const std::string& prefix)
{
    return str.size() >= prefix.size()
        && 0 == str.compare(0, prefix.size(), prefix);
}

}

inline static int findAudioAPIorDefault()
{
    if (vector_audio::shared::availableAudioAPI.find(
            vector_audio::shared::mAudioApi)
        != vector_audio::shared::availableAudioAPI.end())
        return vector_audio::shared::mAudioApi;

    return 0;
}

inline static std::string findHeadsetInputDeviceOrDefault()
{
    if (std::find(vector_audio::shared::availableInputDevices.begin(),
            vector_audio::shared::availableInputDevices.end(),
            vector_audio::shared::configInputDeviceName)
        != vector_audio::shared::availableInputDevices.end())
        return vector_audio::shared::configInputDeviceName;

    return vector_audio::shared::availableInputDevices.front();
}

inline static std::string findHeadsetOutputDeviceOrDefault()
{
    if (std::find(vector_audio::shared::availableOutputDevices.begin(),
            vector_audio::shared::availableOutputDevices.end(),
            vector_audio::shared::configOutputDeviceName)
        != vector_audio::shared::availableOutputDevices.end())
        return vector_audio::shared::configOutputDeviceName;

    return vector_audio::shared::availableOutputDevices.front();
}

inline static std::string findSpeakerOutputDeviceOrDefault()
{
    if (std::find(vector_audio::shared::availableOutputDevices.begin(),
            vector_audio::shared::availableOutputDevices.end(),
            vector_audio::shared::configSpeakerDeviceName)
        != vector_audio::shared::availableOutputDevices.end())
        return vector_audio::shared::configSpeakerDeviceName;

    return vector_audio::shared::availableOutputDevices.front();
}
