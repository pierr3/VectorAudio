#include "application.h"
#include "config.h"
#include "data_file_handler.h"
#include "imgui-SFML.h"
#include "imgui.h"
#include "native/single_instance.h"
#include "native/window_manager.h"
#include "shared.h"
#include "spdlog/spdlog.h"
#include "ui/style.h"
#include "updater.h"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <random>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <string>
#include <thread>

// Main code
int main(int, char**)
{

    std::srand(static_cast<unsigned int>(time(nullptr)));

    vector_audio::SingleInstance instance;
    if (instance.HasRunningInstance()) {
        return 0;
    }

    vector_audio::Configuration::build_logger();
    sf::RenderWindow window(sf::VideoMode(800, 600), "VectorAudio");
    window.setFramerateLimit(30);

    auto image = sf::Image {};

#ifdef SFML_SYSTEM_WINDOWS
    std::string iconName = "icon_win.png";
#else
    std::string iconName = "icon_mac.png";
#endif

    if (!image.loadFromFile(
            (vector_audio::Configuration::get_resource_folder() / iconName)
                .string())) {
        spdlog::error("Could not load application icon");
    } else {
        window.setIcon(
            image.getSize().x, image.getSize().y, image.getPixelsPtr());
    }

    if (!ImGui::SFML::Init(window, false)) {
        spdlog::critical("Could not initialise ImGui SFML");
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
    // Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
    // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    std::filesystem::path p = vector_audio::Configuration::get_resource_folder()
        / std::filesystem::path("JetBrainsMono-Regular.ttf");
    io.Fonts->AddFontFromFileTTF(p.string().c_str(), 18.0);

    if (!ImGui::SFML::UpdateFontTexture()) {
        spdlog::critical("Could not update font textures");
    };

    // Our state

    vector_audio::style::apply_style();
    vector_audio::Configuration::build_config();

    spdlog::info("Starting VectorAudio...");

    auto updaterInstance = std::make_unique<vector_audio::Updater>();

    auto currentApp = std::make_unique<vector_audio::application::App>();

    bool alwaysOnTop = vector_audio::shared::keepWindowOnTop;
    vector_audio::setAlwaysOnTop(window, alwaysOnTop);

    // Main loop
    sf::Clock deltaClock;
    while (window.isOpen()) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
        // tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data
        // to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
        // data to your main application. Generally you may always pass all
        // inputs to dear imgui, and hide them from your application based on
        // those two flags.
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                // Capture the new Ptt key
                if (vector_audio::shared::capturePttFlag) {
                    vector_audio::shared::fallbackPtt
                        = sf::Keyboard::Key::Unknown; // Reset fallback
                    vector_audio::shared::ptt = event.key.scancode;

                    if (vector_audio::shared::ptt
                        == sf::Keyboard::Scan::Unknown) {
                        spdlog::warn("Unknown scancode key when trying to "
                                     "register PTT, falling back to key code");
                        auto fallbackKey
                            = event.key.code; // Fallback to key code
                        if (fallbackKey != sf::Keyboard::Key::Unknown) {

                            vector_audio::shared::ptt
                                = sf::Keyboard::delocalize(fallbackKey);
                            vector_audio::shared::fallbackPtt = fallbackKey;
                            auto keyName = static_cast<std::string>(
                                sf::Keyboard::getDescription(
                                    vector_audio::shared::ptt));
                            spdlog::info("Registered PTT key through "
                                         "delocalized logical key: {}",
                                keyName);

                        } else {
                            spdlog::error(
                                "Could not register PTT key, even with "
                                "fallback.");
                        }
                    }

                    vector_audio::shared::joyStickId = -1;
                    vector_audio::shared::joyStickPtt = -1;
                    vector_audio::Configuration::mConfig["user"]["joyStickId"]
                        = vector_audio::shared::joyStickId;
                    vector_audio::Configuration::mConfig["user"]["joyStickPtt"]
                        = vector_audio::shared::joyStickPtt;
                    vector_audio::Configuration::mConfig["user"]["ptt"]
                        = static_cast<int>(vector_audio::shared::ptt);
                    vector_audio::Configuration::mConfig["user"]["fallbackPtt"]
                        = static_cast<int>(vector_audio::shared::fallbackPtt);
                    vector_audio::Configuration::write_config_async();
                    vector_audio::shared::capturePttFlag = false;
                }
            } else if (event.type == sf::Event::JoystickButtonPressed) {

                if (vector_audio::shared::capturePttFlag) {
                    vector_audio::shared::ptt = sf::Keyboard::Scan::Unknown;
                    vector_audio::shared::fallbackPtt
                        = sf::Keyboard::Key::Unknown;

                    vector_audio::shared::joyStickId
                        = event.joystickButton.joystickId;
                    vector_audio::shared::joyStickPtt
                        = event.joystickButton.button;

                    vector_audio::Configuration::mConfig["user"]["joyStickId"]
                        = vector_audio::shared::joyStickId;
                    vector_audio::Configuration::mConfig["user"]["joyStickPtt"]
                        = vector_audio::shared::joyStickPtt;
                    vector_audio::Configuration::mConfig["user"]["ptt"]
                        = static_cast<int>(vector_audio::shared::ptt);
                    vector_audio::Configuration::write_config_async();
                    vector_audio::shared::capturePttFlag = false;
                }
            }

            if (vector_audio::shared::keepWindowOnTop != alwaysOnTop) {
                vector_audio::setAlwaysOnTop(
                    window, vector_audio::shared::keepWindowOnTop);
                alwaysOnTop = vector_audio::shared::keepWindowOnTop;
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        if (!updaterInstance->need_update())
            currentApp->render_frame();
        else
            updaterInstance->draw();

        // ImGui::ShowDemoWindow(NULL);

        // Rendering
        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    return 0;
}