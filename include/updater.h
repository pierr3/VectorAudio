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

class Updater {
public:
    Updater();

    [[nodiscard]] bool need_update() const;
    void draw();

private:
    bool pNeedUpdate = false;

    std::string pBaseUrl = "https://raw.githubusercontent.com";
    std::string pVersionUrl = "/pierr3/VectorAudio/main/VERSION";
    semver::version pNewVersion;

    std::string pArtefactFileUrl
        = "https://github.com/pierr3/VectorAudio/releases/latest";
    httplib::Client pCli;
};

}