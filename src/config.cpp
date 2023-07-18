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
toml::value configuration::config;

void configuration::build_config()
{
#if defined(SFML_SYSTEM_MACOS)
    std::filesystem::path folder_path = std::filesystem::path(sago::getConfigHome()) / std::filesystem::path("VectorAudio");

    // On macOS we cannot be sure the folder exists as we don't use the folder of
    // the executable, hence we need to create it so we can write the config to it
    if (!std::filesystem::exists(folder_path)) {
        std::filesystem::create_directory(folder_path);
    }

    file_path = folder_path / std::filesystem::path(file_path);
#elif defined(SFML_SYSTEM_LINUX)
    std::string config_path = get_linux_config_folder();
    if (!std::filesystem::exists(config_path)) {
        std::filesystem::create_directory(config_path);
    }
    file_path = config_path + file_path;
#else
    file_path = get_resource_folder() + file_path;
#endif

    airports_db_file_path = get_resource_folder() + airports_db_file_path;

    if (std::filesystem::exists(file_path)) {
        vector_audio::configuration::config = toml::parse(file_path);
    } else {
        spdlog::info("Did not find a config file, starting from scratch.");
    }
}

std::string configuration::get_resource_folder()
{
#ifndef NDEBUG
    return "../resources/";
#else
#ifdef SFML_SYSTEM_MACOS
    return vector_audio::native::osx_resourcePath();
#endif

#ifdef SFML_SYSTEM_LINUX
    /*char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    auto buffer = std::string(result, (count > 0) ? count : 0);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos + 1);*/
    return "../share/vectoraudio/";
#endif

#ifdef SFML_SYSTEM_WINDOWS
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");

    return std::string(buffer).substr(0, pos + 1);
#endif
#endif
}

std::string configuration::get_linux_config_folder()
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

void configuration::write_config_async()
{
    std::thread([]() {
        std::ofstream ofs(vector_audio::configuration::file_path);
        ofs << vector_audio::configuration::config;
        ofs.close();
    }).detach();
}

void configuration::build_logger()
{
    spdlog::init_thread_pool(8192, 1);

    auto async_rotating_file_logger = spdlog::rotating_logger_mt<spdlog::async_factory>(
        "VectorAudio",
        configuration::get_resource_folder() + "vector_audio.log",
        1024 * 1024 * 10, 3);

#ifdef NDEBUG
    spdlog::set_level(spdlog::level::info);
#else
    spdlog::set_level(spdlog::level::trace);
#endif

    spdlog::flush_on(spdlog::level::info);
    spdlog::set_default_logger(async_rotating_file_logger);
}
} // namespace vector_audio