#pragma once
#include "httplib.h"
#include "imgui.h"
#include "platform_folders.h"
#include "shared.h"
#include "spdlog/spdlog.h"
#include "util.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <neargye/semver.hpp>
#include <string>
#include <thread>

namespace vector_audio {

class updater {
public:
    updater();

    bool need_update();
    void draw();

private:
    bool mNeedUpdate;

    std::string mBaseUrl = "https://raw.githubusercontent.com";
    std::string mVersionUrl = "/pierr3/VectorAudio/main/VERSION";
    semver::version mNewVersion;

    std::string mArtefactFileUrl = "https://github.com/pierr3/VectorAudio/releases/latest";
    httplib::Client cli;
};

}