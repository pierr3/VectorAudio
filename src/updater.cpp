#include "updater.h"

namespace vector_audio {

// This class is blocking on purpose, we want to update if needed before
// anything
updater::updater()
    : mNeedUpdate(false)
    , cli(mBaseUrl)
{
    // Check version file
    auto res = cli.Get(mVersionUrl);
    if (!res) {
        spdlog::critical("Cannot access updater endpoint, please update manually!");
        return;
    }

    if (res->status == 200) {
        semver::version currentVersion;

        try {
            currentVersion = semver::version { std::string(VECTOR_VERSION) };
            mNewVersion = semver::version { res->body };
        } catch (std::invalid_argument& ex) {
            spdlog::critical("Cannot parse updater version, please update manually!");
            return;
        }

        if (currentVersion < mNewVersion) {
            mNeedUpdate = true;
        }
    } else {
        spdlog::critical(
            "Updater endpoint did not return ok, please update manually!");
    }
}

bool updater::need_update()
{
#ifdef NDEBUG
    return mNeedUpdate;
#else
    return false;
#endif
}

void updater::draw()
{
    ImGui::Begin("Vector Audio Updater", NULL,
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    ImGui::Text(
        "A new version of Vector Audio is available, please update it! (%s -> "
        "%s)",
        VECTOR_VERSION, mNewVersion.to_string().c_str());

    ImGui::NewLine();
    ImGui::Separator();

    if (ImGui::Button("Download from GitHub")) {
        util::PlatformOpen(mArtefactFileUrl);
    }

    ImGui::End();
}

} // namespace vector_audio