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

    void updater::_downloader() {
        auto downloaderCli = httplib::Client(mArtefactBaseUrl.c_str());
        std::string fileName;

        #ifdef SFML_SYSTEM_MACOS
            fileName = "VectorAudio_"+std::string(VECTOR_ARCH)+".dmg";
        #endif

        #ifdef SFML_SYSTEM_LINUX
            fileName = "VectorAudio_linux.zip";
        #endif

        auto res = cli.Get(std::string(mArtefactFileUrl + fileName).c_str(), [this](uint64_t len, uint64_t total) {
                this->downloadProgress = (len*100/total);
            return true; });
        
        if (res->status == 200) {
            std::string filePath = sago::getDownloadFolder() + "/" + fileName;
            std::ofstream file(filePath);
            file << res->body;
            file.close();

            std::system(std::string(std::string("open ")+filePath).c_str());

            mRequestClose = true;
        }
    }   
        
    bool updater::draw() {
        ImGui::Begin("Vector Audio Updater", NULL, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar 
        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        ImGui::Text("A new version of Vector Audio is available, would you like to download it? (%s -> %s)", VECTOR_VERSION, mNewVersion.c_str());
        
        ImGui::NewLine();
        if (!isDownloading)
            ImGui::NewLine();
        else
            ImGui::Text("Downloading update...");

        ImGui::ProgressBar(downloadProgress, ImVec2(-1.f, 0.f), "Test");
        ImGui::NewLine();

        ImGui::Separator();
        #ifdef SFML_SYSTEM_MACOS
            if (ImGui::Button("Download") && !isDownloading) {
                isDownloading = true;
                auto t = std::thread(&updater::_downloader, this);
                t.detach();
            }
        #else
            ImGui::Text("Please visit github.com/pierr3/VectorAudio to update.");
        #endif


        ImGui::End();

        return mRequestClose;
    }
    
}