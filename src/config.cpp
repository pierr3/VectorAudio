#include "config.h"

#ifdef SFML_SYSTEM_MACOS
#include "osx_resources.h"
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
toml::value Configuration::config_;

void Configuration::build_config()
{
    const auto config_file_path = Configuration::get_config_folder_path() / std::filesystem::path(config_file_name_);

    airports_db_file_path_ = get_resource_folder() / std::filesystem::path(airports_db_file_path_);

    if (std::filesystem::exists(config_file_path)) {
        vector_audio::Configuration::config_ = toml::parse(config_file_path);
    } else {
        spdlog::info("Did not find a config file, starting from scratch.");
    }
}

std::string Configuration::get_resource_folder()
{
#ifndef NDEBUG
    return "../resources/";
#else
#ifdef SFML_SYSTEM_MACOS
    return vector_audio::native::osx_resourcePath();
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
    char* xdg_config_home_chars = getenv("XDG_CONFIG_HOME");
    std::string suffix = "/vector_audio/";
    if (xdg_config_home_chars) {
        std::string xdg_config_home(xdg_config_home_chars);
        return xdg_config_home + suffix;
    }
    std::string home(getenv("HOME"));
    return home + "/.config" + suffix;
}

void Configuration::write_config_async()
{
    std::thread([]() {
        const std::lock_guard<std::mutex> lock(vector_audio::Configuration::config_writer_lock_);
        std::ofstream ofs(Configuration::get_config_folder_path() / std::filesystem::path(config_file_name_));
        ofs << vector_audio::Configuration::config_;
        ofs.close();
    }).detach();
}

void Configuration::build_logger()
{
    spdlog::init_thread_pool(8192, 1);
    auto log_folder = Configuration::get_config_folder_path();

    auto async_rotating_file_logger = spdlog::rotating_logger_mt<spdlog::async_factory>(
        "VectorAudio",
        log_folder / "vector_audio.log",
        1024 * 1024 * 10, 3);

#ifdef NDEBUG
    spdlog::set_level(spdlog::level::info);
#else
    spdlog::set_level(spdlog::level::trace);
#endif

    spdlog::flush_on(spdlog::level::info);
    spdlog::set_default_logger(async_rotating_file_logger);
}

std::filesystem::path Configuration::get_config_folder_path()
{
    std::filesystem::path folder_path;

#if defined(SFML_SYSTEM_MACOS)
    folder_path = std::filesystem::path(sago::getConfigHome()) / std::filesystem::path("VectorAudio");
#elif defined(SFML_SYSTEM_LINUX)
    folder_path = get_linux_config_folder();
#else
    folder_path = get_resource_folder();
#endif

    if (!std::filesystem::exists(folder_path)) {
        std::filesystem::create_directory(folder_path);
    }

    return folder_path;
};
} // namespace vector_audio