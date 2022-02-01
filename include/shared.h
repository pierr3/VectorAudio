#pragma once
#include <string>
#include <vector>

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
    inline std::string callsign = "LFPB_GND";

    inline unsigned int mAudioApi = 0;
    inline std::string configAudioApi;
    inline std::string configInputDeviceName;
    inline std::string configOutputDeviceName;

    inline std::vector<StationElement> FetchedStations;

    // Temp inputs
    inline std::string station_add_callsign = "";
    inline float station_add_frequency = 118.0;
}