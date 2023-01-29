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

namespace vector_audio {

class configuration {
public:
    static toml::value config;

    static inline std::string file_path = "config.toml";
    static inline std::string airports_db_file_path = "airports.json";

    static void build_config();

    static std::string get_resource_folder();

    static void build_logger();

    //
    // TODO fix potential concurrency if the user changes config while still writing
    //
    static void write_config_async();
};

}