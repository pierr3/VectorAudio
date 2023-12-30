#pragma once
#include "platform_folders.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <filesystem>
#include <iostream>
#include <mutex>
#include <SFML/Config.hpp>
#include <string>
#include <thread>
#include <toml.hpp>

namespace vector_audio {

class Configuration {
public:
    static toml::value mConfig;

    static inline std::string mConfigFileName = "config.toml";
    static inline std::string mAirportsDBFilePath = "airports.json";

    static void build_config();

    static std::filesystem::path get_resource_folder();

    static std::string get_linux_config_folder();
    static std::filesystem::path get_config_folder_path();

    inline static std::mutex mConfigWriterLock;

    static void build_logger();

    static void write_config_async();
};

}