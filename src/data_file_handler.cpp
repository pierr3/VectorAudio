#include "shared.h"
#include <data_file_handler.h>
#include <string>

vector_audio::data_file::Handler::Handler()
{
    dataFileThread_ = std::thread(&Handler::thread, this);
    dataFileThread_.detach();
    spdlog::debug("Created data file thread");
}

vector_audio::data_file::Handler::~Handler()
{
    {
        std::unique_lock<std::mutex> lk(m_);
        keep_running_ = false;
    }
    cv_.notify_one();

    if (dataFileThread_.joinable())
        dataFileThread_.join();
};

std::string vector_audio::data_file::Handler::download_string(std::string url, httplib::Client* cli)
{
    auto res = cli->Get(url);
    if (!res) {
        spdlog::error("Could not download URL: {}", url);
        return "";
    }

    if (res->status != 200) {
        spdlog::error("Couldn't load {}, HTTP error {}", url, res->status);
        return "";
    }

    return res->body;
}

bool vector_audio::data_file::Handler::parse_slurper(const std::string& sluper_data)
{
    if (sluper_data.empty()) {
        if (shared::datafile::is_connected)
            spdlog::info("Detected client disconnecting from network");
        vector_audio::shared::datafile::is_connected = false;
        vector_audio::shared::datafile::callsign = "Not connected";

    } else {
        auto lines = shared::split_string(sluper_data, "\n");
        auto res = shared::split_string(lines[0], ",");

        vector_audio::shared::datafile::is_connected = true;
        vector_audio::shared::datafile::callsign = res[1];

        if (util::endsWith(res[1], "_CTR") || util::endsWith(res[1], "_APP")
            || util::endsWith(res[1], "_TWR") || util::endsWith(res[1], "_GND") || util::endsWith(res[1], "_DEL") || util::endsWith(res[1], "_FSS"))
            yx_ = true;

        // Get current user frequency
        int u334 = static_cast<int>(std::atof(res[3].c_str()) * 1000000);
        vector_audio::shared::datafile::frequency = util::cleanUpFrequency(u334);

        vector_audio::shared::datafile::facility = std::stoi(res[2], nullptr, 16) == 10 && yx_ && u334 != shared::kObsFrequency ? 1 : 0;

        vector_audio::shared::slurper::position_lat = std::atof(res[5].c_str());
        vector_audio::shared::slurper::position_lon = std::atof(res[6].c_str());
    }

    return vector_audio::shared::datafile::is_connected;
}
inline void vector_audio::data_file::Handler::thread()
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
                        auto data = status_json["data"]["v3"][0].get<std::string>();
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

                if (!connected_flag && vector_audio::shared::datafile::is_connected) {
                    vector_audio::shared::datafile::is_connected = false;
                    vector_audio::shared::datafile::callsign = "Not connected";
                    spdlog::info("Detected client disconnecting from network");
                }
            }
        }
    } while (!cv_.wait_for(lk, 15s, [this] { return !keep_running_; }));

    spdlog::debug("Data file thread terminated.");
}
