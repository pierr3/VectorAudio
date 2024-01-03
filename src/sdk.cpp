#include "sdk.h"

#include <algorithm>
#include <memory>
#include <restinio/router/express.hpp>
#include <restinio/traits.hpp>

namespace vector_audio {

SDK::SDK(const std::shared_ptr<afv_native::api::atcClient>& clientPtr)
{
    this->pClient = clientPtr;
}

SDK::~SDK()
{
    this->pSDKServer->stop();
    this->pSDKServer.reset();
    this->pRouter.reset();
    this->pClient.reset();
}

bool SDK::start()
{
    try {
        this->buildServer();
        return true;
    } catch (std::exception& ex) {
        spdlog::error("Failed to created SDK http server, is the port in use?");
        spdlog::error("%{}", ex.what());
    }

    return false;
}

void SDK::loopCleanup(const std::vector<std::string>& liveReceivedCallsigns)
{
    // Clear out the old API data every 500ms
    auto currentTime = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - shared::currentlyTransmittingApiTimer)
            .count()
        < 300) {
        return;
    }

    const std::lock_guard<std::mutex> lock(shared::transmittingMutex);
    shared::currentlyTransmittingApiData = "";

    shared::currentlyTransmittingApiData.append(liveReceivedCallsigns.empty()
            ? ""
            : std::accumulate(++liveReceivedCallsigns.begin(),
                liveReceivedCallsigns.end(), *liveReceivedCallsigns.begin(),
                [](auto& a, auto& b) { return a + "," + b; }));
    shared::currentlyTransmittingApiTimer = currentTime;
}

void SDK::buildServer()
{
    this->buildRouter();

    pSDKServer = restinio::run_async<>(restinio::own_io_context(),
        restinio::server_settings_t<serverTraits> {}
            .port(shared::apiServerPort)
            .address("0.0.0.0")
            .request_handler(std::move(this->pRouter)),
        16U);
}

void SDK::buildRouter()
{
    this->pRouter = std::make_unique<restinio::router::express_router_t<>>();

    this->pRouter->http_get(
        mSDKCallUrl[sdkCall::kTransmitting], [&](auto req, auto /*params*/) {
            return SDK::transmittingSDKCall(req);
        });

    this->pRouter->http_get(mSDKCallUrl[sdkCall::kRx],
        [&](auto req, auto /*params*/) { return this->rxSDKCall(req); });

    this->pRouter->http_get(mSDKCallUrl[sdkCall::kTx],
        [&](auto req, auto /*params*/) { return this->txSDKCall(req); });

    this->pRouter->non_matched_request_handler([](auto req) {
        return req->create_response().set_body(shared::kClientName).done();
    });

    auto methodNotAllowed = [](const auto& req, auto) {
        return req->create_response(restinio::status_method_not_allowed())
            .connection_close()
            .done();
    };

    this->pRouter->add_handler(
        restinio::router::none_of_methods(restinio::http_method_get()), "/",
        methodNotAllowed);
}

restinio::request_handling_status_t SDK::transmittingSDKCall(
    const restinio::request_handle_t& req)
{
    const std::lock_guard<std::mutex> lock(shared::transmittingMutex);
    return req->create_response()
        .set_body(shared::currentlyTransmittingApiData)
        .done();
};

restinio::request_handling_status_t SDK::rxSDKCall(
    const restinio::request_handle_t& req)
{
    std::vector<ns::Station> bar;

    std::copy_if(shared::fetchedStations.begin(), shared::fetchedStations.end(),
        std::back_inserter(bar), [this](const ns::Station& s) {
            if (!pClient->IsVoiceConnected())
                return false;
            return pClient->GetRxState(s.getFrequencyHz());
        });

    std::string out;
    if (!bar.empty()) {
        for (auto& f : bar) {
            out += f.getCallsign() + ":" + f.getHumanFrequency() + ",";
        }
    }

    if (out.back() == ',') {
        out.pop_back();
    }

    return req->create_response().set_body(out).done();
};

restinio::request_handling_status_t SDK::txSDKCall(
    const restinio::request_handle_t& req)
{
    std::vector<ns::Station> bar;

    std::copy_if(shared::fetchedStations.begin(), shared::fetchedStations.end(),
        std::back_inserter(bar), [this](const ns::Station& s) {
            if (!pClient->IsVoiceConnected())
                return false;
            return pClient->GetTxState(s.getFrequencyHz());
        });

    std::string out;
    if (!bar.empty()) {
        for (auto& f : bar) {
            out += f.getCallsign() + ":" + f.getHumanFrequency() + ",";
        }
    }

    if (out.back() == ',') {
        out.pop_back();
    }

    return req->create_response().set_body(out).done();
}
}
