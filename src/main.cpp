#include "application.h"
#include "config.h"
#include "data_file_handler.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "keyboardUtil.h"
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
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_image.h>
#include <SDL_video.h>
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

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER
            | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK)
        != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with SDL_Renderer graphics context
    auto windowFlags = static_cast<SDL_WindowFlags>(
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("VectorAudio", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 800, 600, windowFlags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr) {
        SDL_Log("Error creating SDL_Renderer!");
        return 0;
    }

#ifdef SFML_SYSTEM_WINDOWS
    std::string iconPath
        = (vector_audio::Configuration::get_resource_folder() / std::filesystem::path("icon_win.png")).string();
#else
    std::string iconPath
        = (vector_audio::Configuration::get_resource_folder() / std::filesystem::path("icon_mac.png")).string();
#endif

    SDL_Surface* icon = IMG_Load(iconPath.c_str());
    if (icon != NULL) {
        SDL_SetWindowIcon(window, icon);
    } else {
        spdlog::warn("Failed to load app icon: {}", IMG_GetError());
    }

    auto soundPath = vector_audio::Configuration::get_resource_folder()
        / std::filesystem::path("disconnect.wav");

    if (std::filesystem::exists(soundPath)) {
        auto* ret = SDL_LoadWAV(soundPath.c_str(),
            &vector_audio::shared::pDisconnectSoundWavSpec,
            &vector_audio::shared::pDisconnectSoundWavBuffer,
            &vector_audio::shared::pDisconnectSoundWavLength);
        if (ret == nullptr) {
            disconnectWarningSoundAvailable = false;
            spdlog::error(
                "Could not load disconnect sound file: {}", SDL_GetError());
        }
    } else {
        disconnectWarningSoundAvailable = false;
        spdlog::warn("Disconnect sound file not found: {}", soundPath.c_str());
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    std::filesystem::path fontPath
        = vector_audio::Configuration::get_resource_folder()
        / std::filesystem::path("JetBrainsMono-Regular.ttf");
    if (std::filesystem::exists(fontPath)) {
        io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 18.0);
    } else {
        spdlog::warn(
            "Failed to load font, file not found: {}", fontPath.string());
    }

    vector_audio::style::apply_style();
    vector_audio::Configuration::build_config();

    ImVec4 clearColor = ImVec4(0.45F, 0.55F, 0.60F, 1.00F);

    spdlog::info("Starting VectorAudio...");

    auto updaterInstance = std::make_unique<vector_audio::Updater>();

    auto currentApp = std::make_unique<vector_audio::application::App>();

    bool alwaysOnTop = vector_audio::shared::keepWindowOnTop;

    vector_audio::setAlwaysOnTop(window, alwaysOnTop);

    // Main loop
    bool done = false;
    while (!done) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT
                && event.window.event == SDL_WINDOWEVENT_CLOSE
                && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            if (event.type == SDL_KEYDOWN
                && vector_audio::shared::capturePttFlag) {
                auto keyPressed = event.key.keysym.scancode;

                vector_audio::shared::ptt
                    = KeyboardUtil::convertFromSDLToSFML(keyPressed);
                if (vector_audio::shared::ptt
                    == sf::Keyboard::Scancode::Unknown) {
                    spdlog::warn("Unknown scancode key when trying to"
                                 "register PTT, falling back to key"
                                 "code");
                }

                vector_audio::shared::joyStickId = -1;
                vector_audio::shared::joyStickPtt = -1;
                vector_audio::Configuration::mConfig["user"]["joyStickId"]
                    = vector_audio::shared::joyStickId;
                vector_audio::Configuration::mConfig["user"]["joyStickPtt"]
                    = vector_audio::shared::joyStickPtt;
                vector_audio::Configuration::mConfig["user"]["ptt"]
                    = static_cast<int>(vector_audio::shared::ptt);
                vector_audio::Configuration::write_config_async();
                vector_audio::shared::capturePttFlag = false;
            }

            if (event.type == SDL_JOYBUTTONDOWN
                && vector_audio::shared::capturePttFlag) {
                vector_audio::shared::ptt = sf::Keyboard::Scancode::Unknown;

                vector_audio::shared::joyStickId = event.jbutton.which;
                vector_audio::shared::joyStickPtt = event.jbutton.button;

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

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (!updaterInstance->need_update())
            currentApp->render_frame();
        else
            updaterInstance->draw();

        // ImGui::ShowDemoWindow(NULL);

        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x,
            io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(clearColor.x * 255),
            static_cast<Uint8>(clearColor.y * 255),
            static_cast<Uint8>(clearColor.z * 255),
            static_cast<Uint8>(clearColor.w * 255));
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    if (currentApp) {
        currentApp.reset();
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (vector_audio::shared::pDeviceId != 0) {
        SDL_CloseAudioDevice(vector_audio::shared::pDeviceId);
        SDL_FreeWAV(vector_audio::shared::pDisconnectSoundWavBuffer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}