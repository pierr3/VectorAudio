#pragma once
#include "absl/strings/match.h"
#include "httplib.h"
#include "imgui.h"
#include "platform_folders.h"
#include "shared.h"
#include "spdlog/spdlog.h"
#include "util.h"

#include <absl/strings/str_split.h>
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
    static void draw_beta_hint();

    inline static std::string mArtefactFileUrl
        = "https://github.com/pierr3/VectorAudio/releases/latest";
    inline static std::string mAllReleasesUrl
        = "https://github.com/pierr3/VectorAudio/releases";

private:
    bool pNeedUpdate = false;
    bool pIsBetaAvailable = false;

    std::string pBaseUrl = "https://raw.githubusercontent.com";
    std::string pVersionUrl = "/pierr3/VectorAudio/main/VERSION_API";
    semver::version pNewVersion;
    semver::version pBetaVersion;

    httplib::Client pCli;
};

}