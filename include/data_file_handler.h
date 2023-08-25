#pragma once
#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <regex>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

#include <random>
#include <utility>

#include "shared.h"
#include "util.h"
#include <httplib.h>

namespace vector_audio::vatsim {
using namespace std::chrono_literals;

class DataHandler {
public:
    DataHandler();
    virtual ~DataHandler()
    {
        {
            std::unique_lock<std::mutex> lk(m_);
            keep_running_ = false;
        }
        cv_.notify_one();

        if (workerThread_->joinable())
            workerThread_->join();
    };

    bool isSlurperAvailable() const { return this->slurperAvailable_; }

    bool isDatafileAvailable() const { return this->dataFileAvailable_; }

    bool getConnectionStatusWithSlurper();

    bool getConnectionStatusWithDatafile();

private:
    std::regex regex_;
    std::unique_ptr<std::thread> workerThread_;
    std::atomic<bool> keep_running_ = true;
    std::condition_variable cv_;
    std::mutex m_;

    std::string datafile_host_;
    std::string datafile_url_;

    bool slurperAvailable_ = false;
    bool dataFileAvailable_ = false;
    bool had_one_disconnect_ = false;
    bool yx_ = false;

    static std::string downloadString(httplib::Client& cli, std::string url);

    bool parseSlurper(const std::string& sluper_data);

    bool getLatestDatafileURL();

    bool checkIfdatafileAvailable();

    static bool checkIfSlurperAvailable();

    void getAvailableEndpoints();

    static void resetSessionData();

    void handleDisconnect();

    static bool parseDatafile(const std::string& data);

    static void updateSessionInfo(std::string callsign, int frequency = 0,
        int facility = 0, double latitude = 0.0, double longitude = 0.0);

    static void handleConnect();

    void worker();
};
}