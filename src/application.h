#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "AFVatcClientWrapper.h"
#include <thread>
#include <string>
#include <map>
#include <shared_mutex>
#include <memory>
#include "config.h"
#include <iostream>

namespace afv_unix::application {
    class App {
        public:
            App();
            ~App();

            void render_frame();

        private:
            bool mInputFilter;
            bool mOutputEffects;
            float mPeak = 0.0f;
            float mVu = 0.0f;
            int vatsim_cid;
            std::string vatsim_password;

            afv_native::api::atcClient* mClient;

            unsigned int mAudioApi = 0;
            std::string configAudioApi;
            std::string configInputDeviceName;
            std::string configOutputDeviceName;
        };
}