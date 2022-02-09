#include "updater.h"

namespace afv_unix {

    // This class is blocking on purpose, we want to update if needed before anything
    updater::updater() : mNeedUpdate(false), cli(mBaseUrl.c_str()) {
        // Check version file
        auto res = cli.Get(mVersionUrl.c_str());
        if (res->status == 200) {
            if (std::string(res->body) != std::string(VECTOR_VERSION)) {
                mNeedUpdate = true;
                mNewVersion = res->body;
            }
        } else {
            spdlog::error("Could not connect to update server, please update manually");
        }
    }

    bool updater::need_update() {
        return mNeedUpdate;
    }  
        
    void updater::draw() {
        ImGui::Begin("Vector Audio Updater", NULL, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar 
        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        ImGui::Text("A new version of Vector Audio is available, please update it? (%s -> %s)", VECTOR_VERSION, mNewVersion.c_str());

        ImGui::NewLine();

        ImGui::Separator();

        if (ImGui::Button("Download from GitHub")) {
            std::system(std::string(std::string("open ") + mArtefactFileUrl).c_str());
        }

        ImGui::End();
    }
    
}