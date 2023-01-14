#pragma once
#include <string>
#include <vector>
#include <SFML/Window/Keyboard.hpp>
#include <afv-native/hardwareType.h>
#include <map>
#include <chrono>

namespace afv_unix::shared {
    struct StationElement {
        int freq;
        std::string callsign;
        std::string human_freq;

        int transceivers = -1;

        inline static StationElement build(std::string callsign, int freq) {
            StationElement s;
            s.callsign = callsign;
            s.freq = freq;
            
            std::string temp = std::to_string(freq/1000);
            s.human_freq = temp.substr(0, 3) + "." + temp.substr(3, 7);

            return s;
        }
    };

    const std::string client_name = "VectorAudio/"+std::string(VECTOR_VERSION);

    inline bool mInputFilter;
    inline bool mOutputEffects;
    inline float mPeak = 60.0f;
    inline float mVu = 60.0f;
    inline int vatsim_cid;
    inline std::string vatsim_password;

    inline unsigned int mAudioApi = 0;
    inline std::string configAudioApi;
    inline std::string configInputDeviceName;
    inline std::string configOutputDeviceName;
    inline std::string configSpeakerDeviceName;

    inline bool capture_ptt_flag = false;

    inline sf::Keyboard::Key ptt = sf::Keyboard::Unknown;
    inline bool isPttOpen = false;

    inline std::vector<StationElement> FetchedStations;
    inline bool bootUpVccs = false;

    // Temp inputs
    inline std::string station_add_callsign = "";
    inline std::string station_auto_add_callsign = "";
    inline float station_add_frequency = 118.0;

    inline std::vector<int> StationsPendingRemoval;
    inline std::vector<int> StationsPendingRxChange;

    inline afv_native::HardwareType hardware = afv_native::HardwareType::Schmid_ED_137B;

    inline const std::vector<std::string> AvailableHardware = { "Smid ED-137B", "Rockwell Collins 2100", "Garex 220" };

    inline float RadioGain = 1.0f;

    // Temp settings for config window
    inline std::map<unsigned int, std::string> availableAudioAPI;
    inline std::vector<std::string> availableInputDevices;
    inline std::vector<std::string> availableOutputDevices;

    inline static std::string currentlyTransmittingApiData;
    inline static std::chrono::high_resolution_clock::time_point currentlyTransmittingApiTimer;

    inline int apiServerPort = 49080;

    // Thread unsafe stuff
    namespace datafile {
        inline int facility = 0;
        inline bool is_connected = false;

        inline std::string callsign = "Not connected";
        inline int frequency;

        inline std::string atis_callsign = "Not connected";
        inline int atis_frequency;
        inline bool is_atis_connected = false;
        inline std::vector<std::string> atis_text;
    }
}