#pragma once
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "atcClientWrapper.h"
#include "style.h"
#include <thread>
#include <string>
#include <map>
#include <shared_mutex>
#include <memory>
#include <functional>
#include "config.h"
#include <iostream>
#include "modals/settings.h"
#include "shared.h"
#include "event.h"

namespace afv_unix::application {
    class App {
        public:
            App();
            ~App();

            void render_frame();

        private:
            inline bool _frequencyExists(int freq) {
                return std::find_if(shared::FetchedStations.begin(), shared::FetchedStations.end(), 
                    [&freq](const auto& obj) {return obj.freq == freq;}) != shared::FetchedStations.end();
            }

            afv_native::api::atcClient* mClient;

            void _eventCallback(afv_native::ClientEventType evt, void* data, void* data2);
        };
}