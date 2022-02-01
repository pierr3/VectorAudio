#pragma once
#include <string>

namespace afv_unix::shared {
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
}