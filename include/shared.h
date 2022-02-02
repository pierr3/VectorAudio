#pragma once
#include <string>
#include <vector>
#include <SFML/Window/Keyboard.hpp>

namespace afv_unix::shared {
    struct StationElement {
        int freq;
        std::string callsign;

        int transceivers = -1;
    };

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

    inline bool capture_ptt_flag = false;

    inline sf::Keyboard::Key ptt = sf::Keyboard::Unknown;
    inline bool isPttOpen = false;

    inline std::vector<StationElement> FetchedStations;

    // Temp inputs
    inline std::string station_add_callsign = "";
    inline float station_add_frequency = 118.0;

    // Thread unsafe stuff
    namespace datafile {
        inline int rating = 0;
        inline bool is_connected = false;

        inline std::string callsign = "Not connected";
        inline int frequency;
    }
}