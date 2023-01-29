#pragma once
#include "atcClientWrapper.h"
#include "config.h"
#include "event.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "modals/settings.h"
#include "ns/airport.h"
#include "shared.h"
#include "style.h"
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <restinio/all.hpp>
#include <shared_mutex>
#include <string>
#include <thread>

namespace vector_audio::application {
class App {
public:
    App();
    ~App();

    void render_frame();

private:
    inline bool _frequencyExists(int freq)
    {
        return std::find_if(shared::FetchedStations.begin(), shared::FetchedStations.end(),
                   [&freq](const auto& obj) { return obj.freq == freq; })
            != shared::FetchedStations.end();
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