#pragma once
#include "ns/station.h"

#include <afv-native/hardwareType.h>
#include <chrono>
#include <map>
#include <mutex>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <string>
#include <vector>

#define vatsim_status_host "https://status.vatsim.net"
#define vatsim_status_url "/status.json"

#define slurper_host "https://slurper.vatsim.net"
#define slurper_url "/users/info/?cid="

#define url_regex                                                              \
    "^(https?:\\/\\/)?(?:[^@\n]+@)?(?:www\\.)?([^:\\/\n?]+)(\\/[.A-z0-9/-]+)$"

namespace vector_audio::shared {

const std::string kClientName = "VectorAudio/" + std::string(VECTOR_VERSION);

inline bool mInputFilter;
inline bool mOutputEffects;
inline float mPeak = 60.0F;
inline float mVu = 60.0F;
inline int vatsimCid;
inline std::string vatsimPassword;
inline bool keepWindowOnTop = false;

const int kMinVhf = 118000000; // 118.000
const int kMaxVhf = 136975000; // 136.975
const int kObsFrequency = 199998000; // 199.998
const int kUnicomFrequency = 122800000;

inline unsigned int mAudioApi = -1;
inline std::string configAudioApi;
inline std::string configInputDeviceName;
inline std::string configOutputDeviceName;
inline std::string configSpeakerDeviceName;
inline int headsetOutputChannel = 0;

inline bool capturePttFlag = false;

inline sf::Keyboard::Scancode ptt = sf::Keyboard::Scan::Unknown;
inline sf::Keyboard::Key fallbackPtt = sf::Keyboard::Unknown;
inline int joyStickId = -1;
inline int joyStickPtt = -1;
inline bool isPttOpen = false;

inline std::vector<ns::Station> fetchedStations;
inline bool bootUpVccs = false;

// Temp inputs
inline float stationAddFrequency = 118.0;

inline afv_native::HardwareType hardware
    = afv_native::HardwareType::Schmid_ED_137B;

inline const std::vector<std::string> kAvailableHardware
    = { "Smid ED-137B", "Rockwell Collins 2100", "Garex 220" };

inline int radioGain = 100;

// Temp settings for config window
inline std::map<unsigned int, std::string> availableAudioAPI;
inline std::vector<std::string> availableInputDevices;
inline std::vector<std::string> availableOutputDevices;

inline static std::mutex transmittingMutex;
inline static std::string currentlyTransmittingApiData;
inline static std::chrono::high_resolution_clock::time_point
    currentlyTransmittingApiTimer;

inline int apiServerPort = 49080;

// Thread unsafe stuff
namespace session {
    inline std::mutex m;
    inline int facility = 0;
    inline bool isConnected = false;

    inline double latitude;
    inline double longitude;

    inline std::string callsign = "Not connected";
    inline int frequency;
}
}