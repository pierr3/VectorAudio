#include "updater.h"

namespace vector_audio {

// This class is blocking on purpose, we want to update if needed before
// anything
Updater::Updater()
    : pCli(pBaseUrl)
{
    // Check version file
    auto res = pCli.Get(pVersionUrl);
    if (!res) {
        spdlog::critical(
            "Cannot access updater endpoint, please update manually!");
        return;
    }

    if (res->status == 200) {
        semver::version currentVersion;

        try {
            currentVersion = semver::version { std::string(VECTOR_VERSION) };
            pNewVersion = semver::version { res->body };
        } catch (std::invalid_argument& ex) {
            spdlog::critical(
                "Cannot parse updater version, please update manually!");
            spdlog::critical(ex.what());
            return;
        }

        if (currentVersion < pNewVersion) {
            pNeedUpdate = true;
        }
    } else {
        spdlog::critical(
            "Updater endpoint did not return ok, please update manually!");
    }
}

bool Updater::need_update() const
{
#ifdef NDEBUG
    return pNeedUpdate;
#else
    return false;
#endif
}

void Updater::draw()
{
    ImGui::Begin("VectorAudio Updater", NULL,
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    ImGui::Text(
        "A new version of VectorAudio is available, please update it! (%s -> "
        "%s)",
        VECTOR_VERSION, pNewVersion.to_string().c_str());

    ImGui::NewLine();
    ImGui::Separator();

    if (ImGui::Button("Download from GitHub")) {
        util::PlatformOpen(pArtefactFileUrl);
    }

    ImGui::End();
}

} // namespace vector_audio