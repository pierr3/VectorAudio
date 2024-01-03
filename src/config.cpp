#include "config.h"

#include <cstddef>
#include <filesystem>

#ifdef SFML_SYSTEM_MACOS
#include "native/osx_resources.h"
#endif

#ifdef SFML_SYSTEM_WINDOWS
#include <windows.h>
#endif

#ifdef SFML_SYSTEM_LINUX
#include <cstdlib>
#include <limits.h>
#include <unistd.h>
#endif

namespace vector_audio {
toml::value Configuration::mConfig;

void Configuration::build_config()
{
    const auto configFilePath = Configuration::get_config_folder_path()
        / std::filesystem::path(mConfigFileName);

    mAirportsDBFilePath
        = (get_resource_folder() / std::filesystem::path(mAirportsDBFilePath))
              .string();

    if (std::filesystem::exists(configFilePath)) {
        vector_audio::Configuration::mConfig = toml::parse(configFilePath);
    } else {
        spdlog::info("Did not find a config file, starting from scratch.");
    }
}

std::filesystem::path Configuration::get_resource_folder()
{
#ifndef NDEBUG

#ifdef SFML_SYSTEM_MACOS
    return "./";
#else
    return "../resources/";
#endif

#else
#ifdef SFML_SYSTEM_MACOS
    return vector_audio::native::OSXGetResourcePath();
#endif

#ifdef SFML_SYSTEM_LINUX
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    auto buffer = std::string(result, (count > 0) ? count : 0);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    auto currentPath = std::string(buffer).substr(0, pos + 1);
    return currentPath + "../share/vectoraudio/";
#endif

#ifdef SFML_SYSTEM_WINDOWS
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");

    return std::string(buffer).substr(0, pos + 1);
#endif
#endif
}

std::string Configuration::get_linux_config_folder()
{
#ifdef SFML_SYSTEM_LINUX
    char* xdgConfigHomeChars = getenv("XDG_CONFIG_HOME");
    std::string suffix = "/vector_audio/";
    if (xdgConfigHomeChars) {
        std::string xdgConfigHome(xdgConfigHomeChars);
        return xdgConfigHome + suffix;
    }
    std::string home(getenv("HOME"));
    return home + "/.config" + suffix;
#else
    return "";
#endif
}

void Configuration::write_config_async()
{
    std::thread([]() {
        const std::lock_guard<std::mutex> lock(
            vector_audio::Configuration::mConfigWriterLock);
        std::ofstream ofs(Configuration::get_config_folder_path()
            / std::filesystem::path(mConfigFileName));
        ofs << vector_audio::Configuration::mConfig;
        ofs.close();
    }).detach();
}

void Configuration::build_logger()
{
    spdlog::init_thread_pool(8192, 1);
    auto logFolder = Configuration::get_config_folder_path()
        / std::filesystem::path("vector_audio.log");

    auto asyncRotatingFileLogger
        = spdlog::rotating_logger_mt<spdlog::async_factory>("VectorAudio",
            logFolder.string(), static_cast<size_t>(1024 * 1024 * 10), 3);

#ifdef NDEBUG
    spdlog::set_level(spdlog::level::info);
#else
    spdlog::set_level(spdlog::level::trace);
#endif

    spdlog::flush_on(spdlog::level::info);
    spdlog::set_default_logger(asyncRotatingFileLogger);
}

std::filesystem::path Configuration::get_config_folder_path()
{
    std::filesystem::path folderPath;

#if defined(SFML_SYSTEM_MACOS)
    folderPath = std::filesystem::path(sago::getConfigHome())
        / std::filesystem::path("VectorAudio");
#elif defined(SFML_SYSTEM_LINUX)
    folderPath = get_linux_config_folder();
#else
    folderPath = get_resource_folder();
#endif

    if (!std::filesystem::exists(folderPath)) {
        std::filesystem::create_directory(folderPath);
    }

    return folderPath;
};
} // namespace vector_audio