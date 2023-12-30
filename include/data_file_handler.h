#pragma once
#include "shared.h"
#include "util.h"

#include <absl/strings/match.h>
#include <absl/strings/str_split.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <httplib.h>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <random>
#include <regex>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace vector_audio::vatsim {
using namespace std::chrono_literals;

class DataHandler {
public:
    DataHandler();
    virtual ~DataHandler()
    {
        {
            std::unique_lock<std::mutex> lk(pDfMutex);
            pKeepRunning = false;
        }
        pCv.notify_one();

        if (pWorkerThread->joinable())
            pWorkerThread->join();
    };

    bool isSlurperAvailable() const { return this->pSlurperAvailable; }

    bool isDatafileAvailable() const { return this->pDataFileAvailable; }

    bool getConnectionStatusWithSlurper();

    bool getConnectionStatusWithDatafile();

    bool getPilotPositionWithAnything(
        const std::string& callsign, double& latitude, double& longitude);

private:
    std::regex pRegexp;
    std::unique_ptr<std::thread> pWorkerThread;
    std::atomic<bool> pKeepRunning = true;
    std::condition_variable pCv;
    std::mutex pDfMutex;

    std::string pDatafileHost;
    std::string pDatafileUrl;

    bool pSlurperAvailable = false;
    bool pDataFileAvailable = false;
    bool pHadOneDisconnect = false;
    bool pYx = false;

    static std::string downloadString(httplib::Client& cli, std::string url);

    bool parseSlurper(const std::string& sluper_data);

    bool getLatestDatafileURL();

    bool getPilotPositionWithSlurper(
        const std::string& callsign, double& latitude, double& longitude) const;

    bool getPilotPositionWithDatafile(
        const std::string& callsign, double& latitude, double& longitude);

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