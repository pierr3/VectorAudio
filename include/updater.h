#pragma once
#include <string>
#include <thread>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "imgui.h"
#include "httplib.h"
#include "spdlog/spdlog.h"
#include "shared.h"
#include "platform_folders.h"

namespace afv_unix {

    class updater {
        public:
            updater();

            bool need_update();
            bool draw();

        private:
            void _downloader();

            bool mNeedUpdate;

            bool mRequestClose = false;

            float downloadProgress = 0.f;
            bool isDownloading = false;

            std::string mBaseUrl = "https://raw.githubusercontent.com";
            std::string mVersionUrl = "/pierr3/VectorAudio/main/VERSION";
            std::string mNewVersion = "";

            std::string mArtefactBaseUrl = "https://github.com";
            std::string mArtefactFileUrl = "/pierr3/vector_audio/releases/latest/download/";
            httplib::Client cli;
    };
    
}