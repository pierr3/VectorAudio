#include "config.h"

#ifdef SFML_SYSTEM_MACOS
    #include "osx_resources.h"
#endif

#ifdef SFML_SYSTEM_WINDOWS
    #include <windows.h>
#endif

#ifdef SFML_SYSTEM_LINUX
    #include <limits.h>
    #include <unistd.h> 
#endif

namespace afv_unix {
    toml::value configuration::config;

    void configuration::build_config() {
        // TODO: Adjust macOS config file path to ensure it remains after an update
        file_path = get_resource_folder() + file_path;
        airports_db_file_path = get_resource_folder() + airports_db_file_path;

        if (std::filesystem::exists(file_path)) {
            afv_unix::configuration::config = toml::parse(file_path);
        } else {
            spdlog::info("Did not find a config file, starting from scratch.");
        }
    }

    std::string configuration::get_resource_folder() {
        #ifndef NDEBUG
            return "../resources/";
        #else
            #ifdef SFML_SYSTEM_MACOS
                return afv_unix::native::osx_resourcePath();
            #endif

            #ifdef SFML_SYSTEM_LINUX
                char result[PATH_MAX];
                ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
                auto buffer = std::string(result, (count > 0) ? count : 0);
                std::string::size_type pos = std::string(buffer).find_last_of("\\/");
                return std::string(buffer).substr(0, pos+1);
            #endif

            #ifdef SFML_SYSTEM_WINDOWS
                char buffer[MAX_PATH];
                GetModuleFileNameA(NULL, buffer, MAX_PATH);
                std::string::size_type pos = std::string(buffer).find_last_of("\\/");
                
                return std::string(buffer).substr(0, pos+1);
            #endif
        #endif
    }

    void configuration::write_config_async() {
        std::thread([](){
            std::ofstream ofs(afv_unix::configuration::file_path);
            ofs << afv_unix::configuration::config; 
            ofs.close();
        }).detach();
    }

    void configuration::build_logger() {
        spdlog::init_thread_pool(8192, 1);

        auto async_rotating_file_logger = spdlog::rotating_logger_mt<spdlog::async_factory>("VectorAudio", configuration::get_resource_folder() + "vector_audio.log", 1024*1024*10, 3);
        
        #ifdef NDEBUG
            spdlog::set_level(spdlog::level::info);
        #else
            spdlog::set_level(spdlog::level::trace);
        #endif

        spdlog::flush_on(spdlog::level::info);
        spdlog::set_default_logger(async_rotating_file_logger);
    }
}