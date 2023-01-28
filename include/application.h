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
#include <algorithm>
#include <numeric>
#include "config.h"
#include <iostream>
#include "modals/settings.h"
#include "shared.h"
#include "event.h"
#include "ns/airport.h"
#include <thread>
#include <fstream>
#include <restinio/all.hpp>


namespace vector_audio::application {
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
            restinio::running_server_handle_t<restinio::default_traits_t> mSDKServer;

            void _eventCallback(afv_native::ClientEventType evt, void* data, void* data2);
            void _buildSDKServer();
            void _loadAirportsDatabaseAsync();

            bool showErrorModal = false;
            std::string lastErrorModalMessage = "";
        };
}