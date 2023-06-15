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
    inline Handler()
    {
        dataFileThread = std::thread(&Handler::thread, this);
        dataFileThread.detach();
        spdlog::debug("Created data file thread");
    };

    inline virtual ~Handler()
    {
        {
            std::unique_lock<std::mutex> lk(m_);
            _keep_running = false;
        }
        cv.notify_one();

        if (dataFileThread.joinable())
            dataFileThread.join();
    };

    static std::string download_string(std::string url, httplib::Client* cli)
    {
        auto res = cli->Get(url);
        if (!res) {
            spdlog::error("Could not download URL: {}", url);
            return "";
        }

        if (res->status != 200) {
            spdlog::error("Couldn't {}, HTTP error {}", url, res->status);
            return "";
        }

        return res->body;
    }

    static bool parse_slurper(const std::string& sluper_data)
    {
        if (sluper_data.empty()) {
            vector_audio::shared::datafile::is_connected = false;
        } else {
            auto lines = shared::split_string(sluper_data, "\n");
            auto res = shared::split_string(lines[0], ",");

            vector_audio::shared::datafile::is_connected = true;
            vector_audio::shared::datafile::callsign = res[1];

            vector_audio::shared::datafile::facility = res[2] == "atc" ? 1 : 0;

            // Get current user frequency
            int temp_freq = static_cast<int>(std::atof(res[3].c_str()) * 1000000);
            vector_audio::shared::datafile::frequency = util::cleanUpFrequency(temp_freq);

            vector_audio::shared::slurper::position_lat = std::atof(res[5].c_str());
            vector_audio::shared::slurper::position_lon = std::atof(res[6].c_str());
        }

        return vector_audio::shared::datafile::is_connected;
    }

private:
    std::regex regex_ = std::regex();
    std::thread dataFileThread;
    bool _keep_running = true;
    std::condition_variable cv;
    std::mutex m_;

    inline void thread()
    {

        std::string vatsim_datafile_host;
        std::string vatsim_datafile_url;
        //
        // We first check if the slurper is available, if it fails we fallback to datafeed
        //
        auto* slurper_cli = new httplib::Client(slurper_host);
        auto slurper_check = download_string(slurper_url, slurper_cli);
        if (slurper_check.empty()) {
            shared::slurper::is_unavailable = true;
            spdlog::error("Sluper is not available, reverting to datafile");
        }

        //
        // We also get the current datafeed json URL for use later if we need
        //
        auto* vatsim_status_cli = new httplib::Client(vatsim_status_host);
        auto vatsim_status = download_string(vatsim_status_url, vatsim_status_cli);
        if (!vatsim_status.empty()) {
            if (nlohmann::json::accept(vatsim_status)) {
                auto status_json = nlohmann::json::parse(vatsim_status);

                if (status_json.contains("data")) {
                    if (status_json["data"].contains("v3")) {
                        if (status_json["data"]["v3"].is_array()) {
                            std::regex regex(url_regex);
                            std::smatch m;
                            // TODO(pierre): Try and randomize the URL for redundancy
                            auto data = std::string(status_json["data"]["v3"][0]);
                            std::regex_match(data, m, regex);
                            if (m.size() == 4) {
                                vatsim_datafile_host = m[1].str() + m[2].str();
                                vatsim_datafile_url = m[3].str();
                            } else {
                                shared::datafile::is_unavailable = true;
                            }

                        } else {
                            shared::datafile::is_unavailable = true;
                        }
                    } else {
                        shared::datafile::is_unavailable = true;
                    }
                } else {
                    shared::datafile::is_unavailable = true;
                }

                if (shared::datafile::is_unavailable) {
                    spdlog::error("Status file does not have a valid format, cannot use datafile!");
                }
            } else {
                shared::datafile::is_unavailable = true;
                spdlog::error("Status file is not valid json, cannot use datafile!");
            }
        } else {
            shared::datafile::is_unavailable = true;
            spdlog::error("Datafile is not available!");
        }

        if (shared::datafile::is_unavailable && shared::slurper::is_unavailable) {
            spdlog::error("No vatsim datasource available, cannot use program");
            return;
        }

        auto* datafile_cli = new httplib::Client(vatsim_datafile_host);

        std::unique_lock<std::mutex> lk(m_);
        do {

            if (!shared::slurper::is_unavailable) {
                auto sluper_data = download_string(slurper_url + std::to_string(shared::vatsim_cid), slurper_cli);
                parse_slurper(sluper_data);
            }

            // We check the datafile because the slurper is not available
            if (shared::slurper::is_unavailable) {
                auto j3 = nlohmann::json::parse(download_string(vatsim_datafile_url, datafile_cli));
                if (j3["controllers"].is_array()) {

                    bool connected_flag = false;
                    bool atis_connected_flag = false;

                    for (auto controller : j3["controllers"]) {
                        if (controller["cid"] == vector_audio::shared::vatsim_cid) {

                            vector_audio::shared::datafile::callsign = controller["callsign"].get<std::string>();
                            vector_audio::shared::datafile::facility = controller["facility"].get<int>();

                            // Get current user frequency
                            int temp_freq = static_cast<int>(std::atof(controller["frequency"].get<std::string>().c_str()) * 1000000);
                            vector_audio::shared::datafile::frequency = util::cleanUpFrequency(temp_freq);

                            vector_audio::shared::datafile::is_connected = true;
                            connected_flag = true;
                            break;
                        }
                    }

                    for (auto atis : j3["atis"]) {
                        if (atis["cid"] == vector_audio::shared::vatsim_cid) {

                            vector_audio::shared::datafile::atis_callsign = atis["callsign"].get<std::string>();
                            vector_audio::shared::datafile::atis_frequency = std::atof(atis["frequency"].get<std::string>().c_str()) * 1000000;
                            if (atis["text_atis"].is_array()) {
                                vector_audio::shared::datafile::atis_text.clear();
                                for (auto atis_line : atis["text_atis"])
                                    vector_audio::shared::datafile::atis_text.push_back(atis_line.get<std::string>());
                            }
                            atis_connected_flag = true;
                            break;
                        }
                    }

                    if (!connected_flag && vector_audio::shared::datafile::is_connected) {
                        vector_audio::shared::datafile::is_connected = false;
                        spdlog::info("Detected client disconnecting from network");
                    }

                    if (!atis_connected_flag && vector_audio::shared::datafile::is_atis_connected) {
                        vector_audio::shared::datafile::is_atis_connected = false;
                        vector_audio::shared::datafile::atis_callsign = "";
                        vector_audio::shared::datafile::atis_frequency = 0;
                        spdlog::info("Detected atis disconnecting from network");
                    }
                }
            }
        } while (!cv.wait_for(lk, 15s, [this] { return !_keep_running; }));

        spdlog::debug("Data file thread terminated.");
    }
};
}