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
            void draw();

        private:
            bool mNeedUpdate;

            std::string mBaseUrl = "https://raw.githubusercontent.com";
            std::string mVersionUrl = "/pierr3/VectorAudio/main/VERSION";
            std::string mNewVersion = "";

            std::string mArtefactFileUrl = "https://github.com/pierr3/VectorAudio/releases/latest";
            httplib::Client cli;
    };
    
}