#include <data_file_handler.h>
#include <spdlog/spdlog.h>

vector_audio::vatsim::DataHandler::DataHandler()
    : workerThread_(std::make_unique<std::thread>(&DataHandler::worker, this))
{
    spdlog::debug("Created data file thread");
}

std::string vector_audio::vatsim::DataHandler::downloadString(
    httplib::Client& cli, std::string url)
{
    auto res = cli.Get(url);
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

bool vector_audio::vatsim::DataHandler::parseSlurper(
    const std::string& sluper_data)
{
    if (sluper_data.empty()) {
        return false;
    }

    auto lines = shared::split_string(sluper_data, "\n");
    std::string callsign;
    std::string res3;
    std::string res2;
    std::string lat;
    std::string lon;
    bool found_not_atis_connection = false;

    for (const auto& line : lines) {
        if (line.empty()) {
            continue;
        }

        auto res = shared::split_string(line, ",");

        if (util::endsWith(res[1], "_ATIS")) {
            continue; // Ignore ATIS connections
        }

        found_not_atis_connection = true;

        if (util::endsWith(res[1], "_CTR") || util::endsWith(res[1], "_APP")
            || util::endsWith(res[1], "_TWR") || util::endsWith(res[1], "_GND")
            || util::endsWith(res[1], "_DEL") || util::endsWith(res[1], "_FSS")
            || util::endsWith(res[1], "_SUP"))
            yx_ = true;

        callsign = res[1];
        res3 = res[3];
        res2 = res[2];

        lat = res[5];
        lon = res[6];

        break;
    }

    if (callsign == "DCLIENT3") {
        return false;
    }

    if (!found_not_atis_connection) {
        return false;
    }

    int u334 = static_cast<int>(std::atof(res3.c_str()) * 1000000);

    int k422 = std::stoi(res2, nullptr, 16) == 10 && yx_
            && u334 != shared::kObsFrequency
        ? 1
        : 0;

    if (shared::session::is_connected
        && shared::session::callsign != callsign) {
        spdlog::warn(
            "Detected an active session but with a different callsign");
        return false; // If the callsign changes during an active session, we
                      // disconnect
    }

    vector_audio::vatsim::DataHandler::updateSessionInfo(callsign,
        util::cleanUpFrequency(u334), k422, std::atof(lat.c_str()),
        std::atof(lon.c_str()));

    return true;
}
bool vector_audio::vatsim::DataHandler::getLatestDatafileURL()
{
    auto cli = httplib::Client(vatsim_status_host);
    auto res = this->downloadString(cli, vatsim_status_url);

    try {
        if (!nlohmann::json::accept(res)) {
            return false;
        }

        auto status_json = nlohmann::json::parse(res);

        auto number_of_status_files
            = status_json["data"]["v3"].get<std::vector<std::string>>().size();

        auto selected_file_index = number_of_status_files == 1
            ? 0
            : std::rand() % number_of_status_files;

        auto data
            = status_json["data"]["v3"][selected_file_index].get<std::string>();

        std::regex regex(url_regex);
        std::smatch m;
        std::regex_match(data, m, regex);
        if (m.size() == 4) {
            datafile_host_ = m[1].str() + m[2].str();
            datafile_url_ = m[3].str();
            return true;
        }
    } catch (std::exception& e) {
        spdlog::error("Status file check failed: %s", e.what());
    }

    return false;
}
bool vector_audio::vatsim::DataHandler::checkIfdatafileAvailable()
{
    auto cli = httplib::Client(this->datafile_host_);
    auto res = vector_audio::vatsim::DataHandler::downloadString(
        cli, this->datafile_url_);

    return !res.empty();
};
bool vector_audio::vatsim::DataHandler::checkIfSlurperAvailable()
{
    auto cli = httplib::Client(slurper_host);
    auto res
        = vector_audio::vatsim::DataHandler::downloadString(cli, slurper_url);

    return res == "Must Provide CID";
};
void vector_audio::vatsim::DataHandler::getAvailableEndpoints()
{
    auto status_available = this->getLatestDatafileURL();
    if (status_available) {
        this->dataFileAvailable_ = this->checkIfdatafileAvailable();
    }

    this->slurperAvailable_
        = vector_audio::vatsim::DataHandler::checkIfSlurperAvailable();
}
void vector_audio::vatsim::DataHandler::resetSessionData()
{
    shared::session::is_connected = false;
    shared::session::callsign.clear();
    shared::session::facility = 0;
    shared::session::frequency = 0;

    shared::session::latitude = 0.0;
    shared::session::longitude = 0.0;
}
void vector_audio::vatsim::DataHandler::handleDisconnect()
{
    const std::lock_guard<std::mutex> l(shared::session::m);
    if (!shared::session::is_connected) {
        return;
    }

    if (this->had_one_disconnect_) {
        spdlog::info("VATSIM client disconnection confirmed");
        this->had_one_disconnect_ = false;
        vector_audio::vatsim::DataHandler::resetSessionData();
        return;
    }
    spdlog::info("Detected VATSIM client disconnect, waiting for second "
                 "confirmation.");

    this->had_one_disconnect_ = true;
};
bool vector_audio::vatsim::DataHandler::parseDatafile(const std::string& data)
{
    try {
        if (!nlohmann::json::accept(data)) {
            spdlog::error("Failed to parse datafile: not valid JSON");
            return false;
        }

        auto v3_datafile = nlohmann::json::parse(data);

        for (auto controller : v3_datafile["controllers"]) {
            if (controller["cid"] == vector_audio::shared::vatsim_cid) {

                auto callsign = controller["callsign"].get<std::string>();
                if (shared::session::is_connected
                    && shared::session::callsign != callsign) {
                    spdlog::warn("Detected an active session but with a "
                                 "different callsign, disconnecting");
                    return false; // If the callsign changes during an
                                  // active session, we disconnect
                }

                // Get current user frequency
                int u334 = static_cast<int>(
                    std::atof(
                        controller["frequency"].get<std::string>().c_str())
                    * 1000000);

                vector_audio::vatsim::DataHandler::updateSessionInfo(callsign,
                    util::cleanUpFrequency(u334),
                    controller["facility"].get<int>());

                return true;
            }
        }
    } catch (std::exception& e) {
        spdlog::error("Failed to parse datafile: %s", e.what());
    }

    return false;
}
void vector_audio::vatsim::DataHandler::updateSessionInfo(std::string callsign,
    int frequency, int facility, double latitude, double longitude)
{
    const std::lock_guard<std::mutex> l(shared::session::m);
    shared::session::callsign = std::move(callsign);
    shared::session::facility = facility;
    shared::session::latitude = latitude;
    shared::session::longitude = longitude;
    shared::session::frequency = frequency;
}
void vector_audio::vatsim::DataHandler::handleConnect()
{
    const std::lock_guard<std::mutex> l(shared::session::m);
    if (shared::session::is_connected) {
        return;
    }

    spdlog::info("Detected VATSIM client connection");
    shared::session::is_connected = true;
}
void vector_audio::vatsim::DataHandler::worker()
{
    {
        const std::lock_guard<std::mutex> l(shared::session::m);
        this->getAvailableEndpoints();
    }

    std::unique_lock<std::mutex> lk(m_);
    do {
        if (!this->isSlurperAvailable() || !this->isDatafileAvailable()) {
            this->getAvailableEndpoints();
        }

        auto res = false;

        if (this->isSlurperAvailable()) {
            res = this->getConnectionStatusWithSlurper();
        } else if (this->isDatafileAvailable()) {
            res = this->getConnectionStatusWithDatafile();
        }

        if (!res) {
            handleDisconnect();
        } else {
            handleConnect();
        }

    } while (!cv_.wait_for(lk, 15s, [this] { return !keep_running_; }));
}
bool vector_audio::vatsim::DataHandler::getConnectionStatusWithSlurper()
{
    if (!this->isSlurperAvailable()) {
        return false;
    }

    if (shared::vatsim_cid == 0) {
        return false;
    }

    auto cli = httplib::Client(slurper_host);
    std::string res;
    {
        const std::lock_guard<std::mutex> l(shared::session::m);
        std::string url_with_params
            = std::string(slurper_url) + std::to_string(shared::vatsim_cid);
        res = vector_audio::vatsim::DataHandler::downloadString(
            cli, url_with_params);
    }

    return this->parseSlurper(res);
}
bool vector_audio::vatsim::DataHandler::getConnectionStatusWithDatafile()
{
    if (!this->isDatafileAvailable()) {
        return false;
    }

    auto cli = httplib::Client(this->datafile_host_);
    std::string res = vector_audio::vatsim::DataHandler::downloadString(
        cli, this->datafile_url_);

    return this->parseDatafile(res);
}
bool vector_audio::vatsim::DataHandler::getPilotPositionWithSlurper(
    const std::string& callsign, double& latitude, double& longitude)
{
    if (!this->isSlurperAvailable()) {
        return false;
    }

    auto cli = httplib::Client(slurper_host);
    std::string res;
    std::string url_with_params = std::string(slurper_url) + callsign;
    res = vector_audio::vatsim::DataHandler::downloadString(
        cli, url_with_params);

    if (res.empty()) {
        return false;
    }

    try {
        auto lines = shared::split_string(res, "\n");
        for (const auto& line : lines) {
            auto splits = shared::split_string(line, ",");

            if (splits[2] == "pilot") {
                latitude = std::stod(splits[5]);
                longitude = std::stod(splits[6]);

                return true;
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("Error parsing pilot slurper position: %s", e.what());
    }

    return false;
}
bool vector_audio::vatsim::DataHandler::getPilotPositionWithDatafile(
    const std::string& callsign, double& latitude, double& longitude)
{
    if (!this->isDatafileAvailable()) {
        return false;
    }

    auto cli = httplib::Client(this->datafile_host_);
    std::string res = vector_audio::vatsim::DataHandler::downloadString(
        cli, this->datafile_url_);

    if (res.empty()) {
        return false;
    }

    try {
        if (!nlohmann::json::accept(res)) {
            spdlog::error("Failed to parse pilot datafile: not valid JSON");
            return false;
        }

        auto v3_datafile = nlohmann::json::parse(res);

        for (auto pilot : v3_datafile["pilots"]) {
            if (pilot["callsign"] == callsign) {

                latitude = pilot["latitude"].get<double>();
                longitude = pilot["longitude"].get<double>();

                return true;
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse pilot datafile: %s", e.what());
    }

    return false;
}
bool vector_audio::vatsim::DataHandler::getPilotPositionWithAnything(
    const std::string& callsign, double& latitude, double& longitude)
{
    if (this->slurperAvailable_) {
        return this->getPilotPositionWithSlurper(callsign, latitude, longitude);
    }

    if (this->dataFileAvailable_) {
        return this->getPilotPositionWithDatafile(
            callsign, latitude, longitude);
    }

    return false;
}
