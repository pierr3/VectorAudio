#pragma once
#include "platform_folders.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <SFML/Config.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <toml.hpp>
#include <mutex>

namespace vector_audio {

class Configuration {
public:
    static toml::value config_;

    static inline std::string config_file_name_ = "config.toml";
    static inline std::string airports_db_file_path_ = "airports.json";

    static void build_config();

    static std::string get_resource_folder();

    static std::string get_linux_config_folder();
    static std::filesystem::path get_config_folder_path();

    inline static std::mutex config_writer_lock_;

    static void build_logger();

    static void write_config_async();
};

}