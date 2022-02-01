#pragma once
#include <toml.hpp>
#include <string>
#include <thread>
#include <iostream>

namespace afv_unix {

    class configuration {
        public:
        static toml::value config;

        static inline std::string file_path = "config.toml";

        static void build_config();

        //
        // TODO fix potential concurrency if the user changes config while still writing
        //
        static void write_config_async();
    };
    
}