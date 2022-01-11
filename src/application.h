#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "afv-native/afv/APISession.h"
#include "afv-native/Client.h"
#include <thread>
#include <string>
#include <map>
#include <shared_mutex>
#include <event2/event.h>
#include <memory>
#include "config.h"
#include <iostream>

namespace afv_unix::application {
    class App {
        public:
            App();
            virtual ~App();

            void render_frame();

        private:
            struct event_base *mEventBase;

            bool mInputFilter;
            bool mOutputEffects;
            float mPeak = 0.0f;
            float mVu = 0.0f;
            int vatsim_cid;
            std::string vatsim_password;

            std::shared_ptr<afv_native::Client> mClient;

            std::map<afv_native::audio::AudioDevice::Api,std::string> mAudioProviders;
            std::map<int,afv_native::audio::AudioDevice::DeviceInfo> mInputDevices;
            std::map<int,afv_native::audio::AudioDevice::DeviceInfo> mOutputDevices;
            afv_native::audio::AudioDevice::Api mAudioApi = 0;
            int mInputDevice;
            int mOutputDevice;
            std::string configInputDeviceName;
            std::string configOutputDeviceName;

            std::string nameForAudioApi(afv_native::audio::AudioDevice::Api apiNum);
        };
}