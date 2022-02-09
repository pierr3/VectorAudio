#pragma once
#include <string>
#include <thread>
#include <iostream>
#include <filesystem>
#include "imgui.h"
#include "httplib.h"
#include "spdlog/spdlog.h"
#include "shared.h"

namespace afv_unix {

    class updater {
        public:
            updater();

            bool need_update();
            void draw();

        private:
            bool mNeedUpdate;

            std::string mBaseUrl = "https://raw.githubusercontent.com";
            std::string mVersionUrl = "/pierr3/VectorAudio/main/VERSION";
            std::string mNewVersion = "";

            std::string mArtefactBaseUrl = "https://github.com";
            std::string mArtefactFileUrl = "/pierr3/vector_audio/releases/latest/download/";
            httplib::Client cli;
    };
    
}