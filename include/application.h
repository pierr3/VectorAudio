#pragma once
#include "afv-native/atcClientWrapper.h"
#include "afv-native/event.h"
#include "config.h"
#include "data_file_handler.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "ns/airport.h"
#include "radioSimulation.h"
#include "shared.h"
#include "style.h"
#include "ui/modals/settings.h"
#include "ui/widgets/addstation.widget.h"
#include "ui/widgets/gain.widget.h"
#include "ui/widgets/lastrx.widget.h"
#include "util.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <functional>
#include <httplib.h>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <restinio/all.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Window/Joystick.hpp>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <utility>

namespace vector_audio::application {
class App {
public:
    App();
    ~App();

    void render_frame();

private:
    static bool frequencyExists(int freq);

    void errorModal(std::string message);

    afv_native::api::atcClient* pClient;
    restinio::running_server_handle_t<restinio::default_traits_t> pSDKServer;

    void eventCallback(
        afv_native::ClientEventType evt, void* data, void* data2);
    void buildSDKServer();

    void disconnectAndCleanup();

    void playErrorSound();

    void addNewStation();

    // Used in another thread
    static void loadAirportsDatabaseAsync();

    bool pShowErrorModal = false;
    std::string pLastErrorModalMessage;

    std::unique_ptr<vector_audio::vatsim::DataHandler> pDataHandler;

    bool pManuallyDisconnected = false;
    sf::SoundBuffer pDisconnectWarningSoundbuffer;
    sf::Sound pSoundPlayer;
};
}