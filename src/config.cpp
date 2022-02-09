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
        #ifdef NDEBUG
            file_path = get_resource_folder() + file_path;
        #endif

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
                return std::string(result, (count > 0) ? count : 0);
            #endif

            #ifdef SFML_SYSTEM_WINDOWS
                wchar_t path[MAX_PATH] = { 0 };
                GetModuleFileNameW(NULL, path, MAX_PATH);
                std::wstring ws(path);
                return std::string(ws.begin(), ws.end());
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

        std::vector<spdlog::sink_ptr> sinks;

        #ifndef NDEBUG
            auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
            sinks.push_back(stdout_sink);
        #endif
        
        auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(configuration::get_resource_folder() + "vector_audio.log", 1024*1024*10, 3);
        sinks.push_back(rotating_sink);
        
        auto logger = std::make_shared<spdlog::async_logger>("vectorlogger", sinks.begin(), sinks.end(), 
            spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        spdlog::register_logger(logger);
    }
}