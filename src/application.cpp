#include "application.h"


namespace afv_unix::application {

    App::App() {
        
    }

    App::~App(){
        
    }


    // Main loop
    void App::render_frame() {

        #ifdef IMGUI_HAS_VIEWPORT
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->GetWorkPos());
            ImGui::SetNextWindowSize(viewport->GetWorkSize());
            ImGui::SetNextWindowViewport(viewport->ID);
        #else 
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        #endif
        bool open = true;
        ImGui::Begin("MainWindow", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
        
        // Connect button logic
        ImGui::Button("Connect");

        ImGui::SameLine();

        // Settings modal
        if (ImGui::Button("Settings"))
            ImGui::OpenPopup("Settings Panel");


        // Modal definition 
        if (ImGui::BeginPopupModal("Settings Panel"))
        {

            ImGui::Text("VATSIM Details");
            static int i0 = 123;
            ImGui::InputInt("VATSIM ID", &i0);

            static char password[64] = "password123";
            ImGui::InputText("Password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);

            ImGui::NewLine();

            ImGui::Text("Audio configuration");
            
            


            ImGui::NewLine();
            ImGui::NewLine();

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::SameLine();
            ImGui::Button("Save");

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::Text("Connected as: not connected");


        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::End();

    }

}