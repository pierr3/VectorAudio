#include "config.h"

#ifdef SFML_SYSTEM_MACOS
    #include "osx_resources.h"
#endif

namespace afv_unix {
    toml::value configuration::config;

    void configuration::build_config() {
        #ifdef NDEBUG
            file_path = get_resource_folder() + file_path;
        #endif

        if (std::filesystem::exists(file_path)) {
            afv_unix::configuration::config = toml::parse(file_path);
        }
    }

    std::string configuration::get_resource_folder() {
        #ifndef NDEBUG
            return "../resources";
        #else
            #ifdef SFML_SYSTEM_MACOS
                return afv_unix::native::osx_resourcePath();
            #endif

            #ifdef SFML_SYSTEM_LINUX
                return "";
            #endif

            #ifdef SFML_SYSTEM_WINDOWS
                return "";
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
}