// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <afv-native/Client.h>
#include <event2/event.h>
#include "imgui_stdlib.h"
#include <stdio.h>
#include <SDL.h>
#include <string>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

struct event_base *mEventBase;
std::string mAFVAPIServer = "http://localhost:61000";
std::string mAFVUsername = "";
std::string mAFVPassword = "";
std::string mAFVCallsign = "";


double mClientLatitude = 51.1;
double mClientLongitude = -0.2;
double mClientAltitudeMSLM = 500.0;
double mClientAltitudeAGLM = 500.0;

int mTxRadio = 0;
int mCom1Freq = 134225000;
float mCom1Gain = 1.0f;
int mCom2Freq = 126700000;
float mCom2Gain = 1.0f;
bool mPTT = false;

bool mInputFilter = false;
bool mOutputEffects = false;
float mPeak = 0.0f;
float mVu = 0.0f;

std::shared_ptr<afv_native::Client> mClient;

std::map<afv_native::audio::AudioDevice::Api,std::string> mAudioProviders;
std::map<int,afv_native::audio::AudioDevice::DeviceInfo> mInputDevices;
std::map<int,afv_native::audio::AudioDevice::DeviceInfo> mOutputDevices;
afv_native::audio::AudioDevice::Api mAudioApi;
int mInputDevice;
int mOutputDevice;

std::string nameForAudioApi(afv_native::audio::AudioDevice::Api apiNum) {
    auto mIter = mAudioProviders.find(apiNum);
    if (mIter == mAudioProviders.end()) {
        if (apiNum == 0) return "UNSPECIFIED (default)";
        return "INVALID";
    }
    return mIter->second;
}

void setAudioApi(afv_native::audio::AudioDevice::Api apiNum) {
    mAudioApi = apiNum;
    mInputDevices = afv_native::audio::AudioDevice::getCompatibleInputDevicesForApi(mAudioApi);
    mOutputDevices = afv_native::audio::AudioDevice::getCompatibleOutputDevicesForApi(mAudioApi);
}

void setAudioDevice() {
    mClient->setAudioApi(mAudioApi);
    try {
        mClient->setAudioInputDevice(mInputDevices.at(mInputDevice).id);
    } catch (std::out_of_range &) {}
    try {
        mClient->setAudioOutputDevice(mOutputDevices.at(mOutputDevice).id);
    } catch (std::out_of_range &) {}
}

// Main code
int main(int, char**)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

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
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    mEventBase = event_base_new();

    mClient = std::make_shared<afv_native::Client>(mEventBase,2);

    SDL_SetWindowSize(window, 800, 400);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // update certain things...
        if (mClient) {
            mPeak = mClient->getInputPeak();
            mVu = mClient->getInputVu();
        }

        ImGui::Begin("AFV-Native Test Client");
        ImGui::InputText("AFV API Server URL", &mAFVAPIServer, ImGuiInputTextFlags_CharsNoBlank);
        ImGui::InputText("AFV Username", &mAFVUsername, ImGuiInputTextFlags_CharsNoBlank);
        ImGui::InputText("AFV Password", &mAFVPassword, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_Password);
        ImGui::InputText(
                "AFV Callsign",
                &mAFVCallsign,
                ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsUppercase);
        if (ImGui::CollapsingHeader("Audio Configuration")) {
            if (ImGui::BeginCombo("Sound API", nameForAudioApi(mAudioApi).c_str())) {
                if (ImGui::Selectable("Default", mAudioApi == 0)) {
                    setAudioApi(0);
                }
                for (const auto &item: mAudioProviders) {
                    if (ImGui::Selectable(item.second.c_str(), mAudioApi == item.first)) {
                        setAudioApi(item.first);
                    }
                }
                ImGui::EndCombo();
            }
            const char *currentInputDevice = "None Selected";
            if (mInputDevices.find(mInputDevice) != mInputDevices.end()) {
                currentInputDevice = mInputDevices.at(mInputDevice).name.c_str();
            }
            if (ImGui::BeginCombo("Input Device", currentInputDevice)) {
                for (const auto &pair: mInputDevices) {
                    if (ImGui::Selectable(pair.second.name.c_str(), mInputDevice == pair.first)) {
                        mInputDevice = pair.first;
                    }
                }
                ImGui::EndCombo();
            }
            const char *currentOutputDevice = "None Selected";
            if (mOutputDevices.find(mOutputDevice) != mOutputDevices.end()) {
                currentOutputDevice = mOutputDevices.at(mOutputDevice).name.c_str();
            }
            if (ImGui::BeginCombo("Output Device", currentOutputDevice)) {
                for (const auto &pair: mOutputDevices) {
                    if (ImGui::Selectable(pair.second.name.c_str(), mOutputDevice == pair.first)) {
                        mOutputDevice = pair.first;
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::Checkbox("Enable Input Filter", &mInputFilter)) {
                if (mClient) {
                    mClient->setEnableInputFilters(mInputFilter);
                }
            }
            if (ImGui::Checkbox("Enable Output Effects", &mOutputEffects)) {
                if (mClient) {
                    mClient->setEnableOutputEffects(mOutputEffects);
                }
            }
        }
        if (ImGui::CollapsingHeader("Client Position")) {
            ImGui::InputDouble("Latitude", &mClientLatitude);
            ImGui::InputDouble("Longitude", &mClientLongitude);
            ImGui::InputDouble("Altitude (AMSL) (M)", &mClientAltitudeMSLM);
            ImGui::InputDouble("Height (AGL) (M)", &mClientAltitudeAGLM);
        }

        if (ImGui::CollapsingHeader("COM1", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::InputInt("COM1 Frequency (Hz)", &mCom1Freq, 5000, 1000000)) {
                if (mClient) {
                    mClient->setRadioState(0, mCom1Freq);
                }
            }
            if (ImGui::SliderFloat("COM1 Gain", &mCom1Gain, 0.0f, 1.5f)) {
                mCom1Gain = std::min(1.5f, mCom1Gain);
                mCom1Gain = std::max(0.0f, mCom1Gain);
                if (mClient) {
                    mClient->setRadioGain(0, mCom1Gain);
                }
            }
        }
        if (ImGui::CollapsingHeader("COM2", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::InputInt("COM2 Frequency (Hz)", &mCom2Freq, 5000, 1000000)) {
                if (mClient) {
                    mClient->setRadioState(1, mCom2Freq);
                }
            }
            if (ImGui::SliderFloat("COM2 Gain", &mCom2Gain, 0.0f, 1.5f)) {
                mCom2Gain = std::min(1.5f, mCom2Gain);
                mCom2Gain = std::max(0.0f, mCom2Gain);
                if (mClient) {
                    mClient->setRadioGain(1, mCom2Gain);
                }
            }
        }
        if (ImGui::BeginCombo("Transmission Radio", (mTxRadio == 0) ? "COM1" : "COM2")) {
            if (ImGui::Selectable("COM1", mTxRadio == 0)) {
                mTxRadio = 0;
                if (mClient) {
                    mClient->setTxRadio(mTxRadio);
                }
            }
            if (ImGui::Selectable("COM2", mTxRadio == 1)) {
                mTxRadio = 1;
                if (mClient) {
                    mClient->setTxRadio(mTxRadio);
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Checkbox("Push to Talk", &mPTT)) {
            if (mClient) {
                mClient->setPtt(mPTT);
            }
        }

        const ImVec4 red(1.0, 0.0, 0.0, 1.0), green(0.0, 1.0, 0.0, 1.0);
        ImGui::TextColored(mClient->isAPIConnected() ? green : red, "API Server Connection");
        ImGui::TextColored(mClient->isVoiceConnected() ? green : red, "Voice Server Connection");
        ImGui::SliderFloat("Input VU", &mVu, -60.0f, 0.0f);
        ImGui::SliderFloat("Input Peak", &mPeak, -60.0f, 0.0f);

        if (!mClient->isVoiceConnected()) {
            if (ImGui::Button("Connect Voice")) {
                setAudioDevice();
                mClient->setBaseUrl(mAFVAPIServer);
                mClient->setClientPosition(mClientLatitude, mClientLongitude, mClientAltitudeMSLM, mClientAltitudeAGLM);
                mClient->setRadioState(0, mCom1Freq);
                mClient->setRadioState(1, mCom2Freq);
                mClient->setTxRadio(mTxRadio);
                mClient->setRadioGain(0, mCom1Gain);
                mClient->setRadioGain(1, mCom2Gain);
                mClient->setCredentials(mAFVUsername, mAFVPassword);
                mClient->setCallsign(mAFVCallsign);
                mClient->setEnableInputFilters(mInputFilter);
                mClient->setEnableOutputEffects(mOutputEffects);
                mClient->connect();
            }
        } else {
            if (ImGui::Button("Disconnect")) {
                mClient->disconnect();
            }
        }
        if (ImGui::Button("Start Audio (VU/Peak Test)")) {
            setAudioDevice();
            mClient->startAudio();
        }
        if (ImGui::Button("Stop Audio (VU/Peak Test)")) {
            mClient->stopAudio();
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}