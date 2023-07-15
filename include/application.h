#pragma once
#include "afv-native/atcClientWrapper.h"
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
#include <utility>
#include <httplib.h>
#include <data_file_handler.h>

namespace vector_audio::application {
class App {
public:
    App();
    ~App();

    void render_frame();

private:
    static bool frequencyExists(int freq);

    void errorModal(std::string message);

    afv_native::api::atcClient* mClient_;
    restinio::running_server_handle_t<restinio::default_traits_t> mSDKServer_;

    void eventCallback(afv_native::ClientEventType evt, void* data, void* data2);
    void buildSDKServer();

    // Used in another thread
    static void loadAirportsDatabaseAsync();

    bool showErrorModal_ = false;
    std::string lastErrorModalMessage_;
};
}