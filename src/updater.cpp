#include "updater.h"

#include "shared.h"

#include <spdlog/spdlog.h>

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

    if (res->status != 200) {
        spdlog::critical(
            "Updater endpoint did not return ok, please update manually!");

        return;
    }

    semver::version currentVersion
        = semver::version { std::string(VECTOR_VERSION) };

    try {
        std::string cleanBody = res->body;
        absl::StripAsciiWhitespace(&res->body);
        pNewVersion = semver::version { res->body };
    } catch (std::invalid_argument& ex) {
        spdlog::critical(
            "Cannot parse updater version, please update manually!");
        spdlog::critical(ex.what());
        return;
    }

    if (pNewVersion > currentVersion) {
        pNeedUpdate = true;
        return; // We don't check for beta as this is a mandatory update
    }

    // isBetaAvailable
    auto betaRes = pCli.Get(pBetaVersionUrl);

    if (res->status == 200) {
        spdlog::warn("Cannot access updater beta endpoint!");
        return;
    }

    try {
        std::string cleanBody = betaRes->body;
        absl::StripAsciiWhitespace(&betaRes->body);
        pBetaVersion = semver::version { cleanBody };
    } catch (std::invalid_argument& ex) {
        spdlog::warn("Cannot parse updater beta version!");
        spdlog::warn(ex.what());
        return;
    }

    if (pBetaVersion == currentVersion) {
        // We are using the beta version
        shared::isUsingBeta = true;
        shared::betaVersionString = pBetaVersion.to_string();
    } else if (pBetaVersion > currentVersion) {
        shared::isBetaAvailable = true;
        shared::betaVersionString = pBetaVersion.to_string();
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
    ImGui::Begin("VectorAudio Updater", nullptr,
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
        util::PlatformOpen(mArtefactFileUrl);
    }

    ImGui::End();
}

void Updater::draw_beta_hint()
{
    if (shared::isUsingBeta) {
        ImGui::NewLine();

        ImGui::TextColored(ImColor(30, 140, 45).Value,
            "You are using a\nbeta version.\n"
            "Please report "
            "any\n"
            "issue you\nencounter!");

        ImGui::NewLine();

    } else if (shared::isBetaAvailable) {
        ImGui::NewLine();
        ImGui::TextColored(ImColor(30, 140, 45).Value,
            "New beta version\navailable!\n(%s)",
            shared::betaVersionString.c_str());
        util::TextURL("Download here", mAllReleasesUrl);
        ImGui::NewLine();
    }
}
} // namespace vector_audio