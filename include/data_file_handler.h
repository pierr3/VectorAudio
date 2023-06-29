#pragma once
#include <atomic>
#include <chrono>
#include <nlohmann/json_fwd.hpp>
#include <regex>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

#include "shared.h"
#include "util.h"
#include <httplib.h>
#include <nlohmann/json.hpp>

namespace vector_audio::data_file {
using namespace std::chrono_literals;

class Handler {
public:
    Handler();

    virtual ~Handler();

    static std::string download_string(std::string url, httplib::Client* cli);

    static bool parse_slurper(const std::string& sluper_data);

private:
    std::regex regex_ = std::regex();
    std::thread dataFileThread_;
    bool keep_running_ = true;
    std::condition_variable cv_;
    std::mutex m_;

    static inline bool yx_ = false;

    void thread();
};
}