#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <afv-native/hardwareType.h>
#include <chrono>
#include <map>
#include <string>
#include <utility>
#include <vector>

#define vatsim_status_host "https://status.vatsim.net"
#define vatsim_status_url "/status.json"

#define slurper_host "https://slurper.vatsim.net"
#define slurper_url "/users/info/?cid="

#define url_regex "^(https?:\\/\\/)?(?:[^@\n]+@)?(?:www\\.)?([^:\\/\n?]+)(\\/[.A-z0-9/-]+)$"

namespace vector_audio::shared {
struct StationElement {
    int freq;
    std::string callsign;
    std::string human_freq;

    int transceivers = -1;

    inline static StationElement build(std::string callsign, int freq)
    {
        StationElement s;
        s.callsign = std::move(callsign);
        s.freq = freq;

        std::string temp = std::to_string(freq / 1000);
        s.human_freq = temp.substr(0, 3) + "." + temp.substr(3, 7);

        return s;
    }
};

static std::vector<std::string> split_string(const std::string& str,
                                      const std::string& delimiter)
{
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos)
    {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.size();
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.push_back(str.substr(prev));

    return strings;
}

const std::string kClientName = "VectorAudio/" + std::string(VECTOR_VERSION);

inline bool mInputFilter;
inline bool mOutputEffects;
inline float mPeak = 60.0f;
inline float mVu = 60.0f;
inline int vatsim_cid;
inline std::string vatsim_password;

inline unsigned int mAudioApi = -1;
inline std::string configAudioApi;
inline std::string configInputDeviceName;
inline std::string configOutputDeviceName;
inline std::string configSpeakerDeviceName;
inline int headsetOutputChannel = 0;

inline bool capture_ptt_flag = false;

inline sf::Keyboard::Key ptt = sf::Keyboard::Unknown;
inline int joyStickId = -1;
inline int joyStickPtt = -1;
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

inline int RadioGain = 100;

// Temp settings for config window
inline std::map<unsigned int, std::string> availableAudioAPI;
inline std::vector<std::string> availableInputDevices;
inline std::vector<std::string> availableOutputDevices;

inline static std::string currentlyTransmittingApiData;
inline static std::chrono::high_resolution_clock::time_point currentlyTransmittingApiTimer;

inline int apiServerPort = 49080;

namespace slurper {
    inline bool is_unavailable = false;

    inline float position_lat;
    inline float position_lon;
}

// Thread unsafe stuff
namespace datafile {
    inline int facility = 0;
    inline bool is_connected = false;

    inline bool is_unavailable = false;

    inline std::string callsign = "Not connected";
    inline int frequency;
}
}