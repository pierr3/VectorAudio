#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "imgui.h"
#include "imgui-SFML.h"
#include "style.h"
#include "config.h"
#include "shared.h"
#include "application.h"
#include "data_file_handler.h"
#include <thread>
#include <stdio.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

// Main code
int main(int, char**)
{
    sf::RenderWindow window(sf::VideoMode(800, 400), "VECTOR Audio");
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);    

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state

    afv_unix::style::apply_style();
    afv_unix::configuration::build_config();

    afv_unix::application::App* currentApp = new afv_unix::application::App();

    auto dataFileHandler = new afv_unix::data_file::Handler();

    // Main loop
    sf::Clock deltaClock;
    while (window.isOpen())
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if(event.type == sf::Event::KeyPressed) {

                // Capture the new Ptt key
                if (afv_unix::shared::capture_ptt_flag) {
                    afv_unix::shared::ptt = event.key.code;

                    afv_unix::configuration::config["user"]["ptt"] = static_cast<int>(afv_unix::shared::ptt);
                    afv_unix::configuration::write_config_async();
                    afv_unix::shared::capture_ptt_flag = false;
                }
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        currentApp->render_frame();

        //ImGui::ShowDemoWindow(NULL);

        // Rendering
        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    // Close the datafile thread
    delete dataFileHandler;

    // Cleanup
    delete currentApp;
    ImGui::SFML::Shutdown();

    return 0;
}