
#include <data_file_handler.h>

vector_audio::vatsim::DataHandler::DataHandler()
    : pWorkerThread(std::make_unique<std::thread>(&DataHandler::worker, this))
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

    auto lines = absl::StrSplit(sluper_data, '\n');
    std::string callsign;
    std::string res3;
    std::string res2;
    std::string lat;
    std::string lon;
    bool foundNotAtisConnection = false;

    for (const auto& line : lines) {
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> res = absl::StrSplit(line, ',');

        if (absl::EndsWith(res[1], "_ATIS")) {
            continue; // Ignore ATIS connections
        }

        foundNotAtisConnection = true;

        auto allowedYx = { "_CTR", "_APP", "_TWR", "_GND", "_DEL", "_FSS",
            "_SUP", "_RDO", "_RMP", "_TMU", "_FMP" };

        for (const auto& yxTest : allowedYx) {
            if (absl::EndsWith(res[1], yxTest)) {
                pYx = true;
                break;
            }
        }

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

    if (!foundNotAtisConnection) {
        return false;
    }

    res3.erase(std::remove(res3.begin(), res3.end(), '.'), res3.end());
    int u334 = std::atoi(res3.c_str()) * 1000;

    int k422 = std::stoi(res2, nullptr, 16) == 10 && pYx ? 1 : 0;

    k422 = u334 != shared::kObsFrequency && k422 == 1   ? 1
        : k422 == 1 && absl::EndsWith(callsign, "_SUP") ? 1
                                                        : 0;

    if (shared::session::isConnected && shared::session::callsign != callsign) {
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

        auto statusJson = nlohmann::json::parse(res);

        auto numberOfStatusFiles
            = statusJson["data"]["v3"].get<std::vector<std::string>>().size();

        auto selectedFileIndex
            = numberOfStatusFiles == 1 ? 0 : std::rand() % numberOfStatusFiles;

        auto data
            = statusJson["data"]["v3"][selectedFileIndex].get<std::string>();

        std::regex regex(url_regex);
        std::smatch m;
        std::regex_match(data, m, regex);
        if (m.size() == 4) {
            pDatafileHost = m[1].str() + m[2].str();
            pDatafileUrl = m[3].str();
            return true;
        }
    } catch (std::exception& e) {
        spdlog::error("Status file check failed: %s", e.what());
    }

    return false;
}

bool vector_audio::vatsim::DataHandler::checkIfdatafileAvailable()
{
    auto cli = httplib::Client(this->pDatafileHost);
    auto res = vector_audio::vatsim::DataHandler::downloadString(
        cli, this->pDatafileUrl);

    return !res.empty();
}

bool vector_audio::vatsim::DataHandler::checkIfSlurperAvailable()
{
    auto cli = httplib::Client(slurper_host);
    auto res
        = vector_audio::vatsim::DataHandler::downloadString(cli, slurper_url);

    return res == "Must Provide CID";
}

void vector_audio::vatsim::DataHandler::getAvailableEndpoints()
{
    auto statusAvailable = this->getLatestDatafileURL();
    if (statusAvailable) {
        this->pDataFileAvailable = this->checkIfdatafileAvailable();
    }

    this->pSlurperAvailable
        = vector_audio::vatsim::DataHandler::checkIfSlurperAvailable();
}

void vector_audio::vatsim::DataHandler::resetSessionData()
{
    shared::session::isConnected = false;
    shared::session::callsign.clear();
    shared::session::facility = 0;
    shared::session::frequency = 0;

    shared::session::latitude = 0.0;
    shared::session::longitude = 0.0;
}

void vector_audio::vatsim::DataHandler::handleDisconnect()
{
    const std::lock_guard<std::mutex> l(shared::session::m);
    if (!shared::session::isConnected) {
        return;
    }

    if (this->pHadOneDisconnect) {
        spdlog::info("VATSIM client disconnection confirmed");
        this->pHadOneDisconnect = false;
        vector_audio::vatsim::DataHandler::resetSessionData();
        return;
    }
    spdlog::info("Detected VATSIM client disconnect, waiting for second "
                 "confirmation.");

    this->pHadOneDisconnect = true;
}

bool vector_audio::vatsim::DataHandler::parseDatafile(const std::string& data)
{
    try {
        if (!nlohmann::json::accept(data)) {
            spdlog::error("Failed to parse datafile: not valid JSON");
            return false;
        }

        auto v3Datafile = nlohmann::json::parse(data);

        for (auto controller : v3Datafile["controllers"]) {
            if (controller["cid"] == vector_audio::shared::vatsimCid) {

                auto callsign = controller["callsign"].get<std::string>();
                if (shared::session::isConnected
                    && shared::session::callsign != callsign) {
                    spdlog::warn("Detected an active session but with a "
                                 "different callsign, disconnecting");
                    return false; // If the callsign changes during an
                                  // active session, we disconnect
                }

                auto res3 = controller["frequency"].get<std::string>();
                res3.erase(
                    std::remove(res3.begin(), res3.end(), '.'), res3.end());
                int u334 = std::atoi(res3.c_str()) * 1000;

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
    if (shared::session::isConnected) {
        return;
    }

    spdlog::info("Detected VATSIM client connection");
    shared::session::isConnected = true;
}

void vector_audio::vatsim::DataHandler::worker()
{
    {
        const std::lock_guard<std::mutex> l(shared::session::m);
        this->getAvailableEndpoints();
    }

    std::unique_lock<std::mutex> lk(pDfMutex);
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

    } while (!pCv.wait_for(lk, 15s, [this] { return !pKeepRunning; }));
}

bool vector_audio::vatsim::DataHandler::getConnectionStatusWithSlurper()
{
    if (!this->isSlurperAvailable()) {
        return false;
    }

    if (shared::vatsimCid == 0) {
        return false;
    }

    auto cli = httplib::Client(slurper_host);
    std::string res;
    {
        const std::lock_guard<std::mutex> l(shared::session::m);
        std::string urlWithParams
            = std::string(slurper_url) + std::to_string(shared::vatsimCid);
        res = vector_audio::vatsim::DataHandler::downloadString(
            cli, urlWithParams);
    }

    return this->parseSlurper(res);
}

bool vector_audio::vatsim::DataHandler::getConnectionStatusWithDatafile()
{
    if (!this->isDatafileAvailable()) {
        return false;
    }

    auto cli = httplib::Client(this->pDatafileHost);
    std::string res = vector_audio::vatsim::DataHandler::downloadString(
        cli, this->pDatafileUrl);

    return this->parseDatafile(res);
}

bool vector_audio::vatsim::DataHandler::getPilotPositionWithSlurper(
    const std::string& callsign, double& latitude, double& longitude) const
{
    if (!this->isSlurperAvailable()) {
        return false;
    }

    auto cli = httplib::Client(slurper_host);
    std::string res;
    std::string urlWithParams = std::string(slurper_url) + callsign;
    res = vector_audio::vatsim::DataHandler::downloadString(cli, urlWithParams);

    if (res.empty()) {
        return false;
    }

    try {
        auto lines = absl::StrSplit(res, '\n');
        for (const auto& line : lines) {
            std::vector<std::string> splits = absl::StrSplit(line, ',');

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

    auto cli = httplib::Client(this->pDatafileHost);
    std::string res = vector_audio::vatsim::DataHandler::downloadString(
        cli, this->pDatafileUrl);

    if (res.empty()) {
        return false;
    }

    try {
        if (!nlohmann::json::accept(res)) {
            spdlog::error("Failed to parse pilot datafile: not valid JSON");
            return false;
        }

        auto v3Datafile = nlohmann::json::parse(res);

        for (auto pilot : v3Datafile["pilots"]) {
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
    if (this->pSlurperAvailable) {
        return this->getPilotPositionWithSlurper(callsign, latitude, longitude);
    }

    if (this->pDataFileAvailable) {
        return this->getPilotPositionWithDatafile(
            callsign, latitude, longitude);
    }

    return false;
}
