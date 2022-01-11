#include "imgui.h"
#include "afv-native/Client.h"
#include <thread>
#include <string>
#include <map>
#include <shared_mutex>
#include <event2/event.h>
#include <memory>

namespace afv_unix::application {
    class App {
        public:
            App();
            virtual ~App();

            void render_frame();

        private:
            struct event_base *mEventBase;

            bool mInputFilter = true;
            bool mOutputEffects = true;
            float mPeak = 0.0f;
            float mVu = 0.0f;

            std::shared_ptr<afv_native::ATCClient> mClient;

            std::map<afv_native::audio::AudioDevice::Api,std::string> mAudioProviders;
            std::map<int,afv_native::audio::AudioDevice::DeviceInfo> mInputDevices;
            std::map<int,afv_native::audio::AudioDevice::DeviceInfo> mOutputDevices;
            afv_native::audio::AudioDevice::Api mAudioApi;
            int mInputDevice;
            int mOutputDevice;

            std::string nameForAudioApi(afv_native::audio::AudioDevice::Api apiNum);

            afv_native::audio::AudioDevice::Api m_audioApi = 0;
        };
}