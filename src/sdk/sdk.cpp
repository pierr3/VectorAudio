#include "sdk/sdk.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>
#include <restinio/router/express.hpp>
#include <restinio/traits.hpp>

namespace vector_audio {

SDK::SDK(const std::shared_ptr<afv_native::api::atcClient>& clientPtr)
{
    this->pClient = clientPtr;
}

SDK::~SDK()
{
    for (auto [id, ws] : this->pWsRegistry) {
        ws->shutdown();
        ws.reset();
    }
    this->pWsRegistry.clear();
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

void SDK::handleAFVEventForWebsocket(sdk::types::Event event,
    const std::optional<std::string>& callsign,
    const std::optional<int>& frequencyHz)
{
    if (!this->pSDKServer || !this->pClient->IsVoiceConnected()) {
        return;
    }

    if (event == sdk::types::Event::kRxBegin && callsign && frequencyHz) {
        nlohmann::json jsonMessage
            = WebsocketMessage::buildMessage(WebsocketMessageType::kRxBegin);
        jsonMessage["value"]["callsign"] = *callsign;
        jsonMessage["value"]["pFrequencyHz"] = *frequencyHz;
        this->broadcastOnWebsocket(jsonMessage.dump());
        return;
    }

    if (event == sdk::types::Event::kRxEnd && callsign && frequencyHz) {
        nlohmann::json jsonMessage
            = WebsocketMessage::buildMessage(WebsocketMessageType::kRxEnd);
        jsonMessage["value"]["callsign"] = *callsign;
        jsonMessage["value"]["pFrequencyHz"] = *frequencyHz;
        this->broadcastOnWebsocket(jsonMessage.dump());
        return;
    }

    if (event == sdk::types::Event::kFrequencyStateUpdate) {
        nlohmann::json jsonMessage = WebsocketMessage::buildMessage(
            WebsocketMessageType::kFrequencyStateUpdate);

        std::vector<ns::Station> rxBar;
        std::vector<ns::Station> txBar;
        std::vector<ns::Station> xcBar;

        // std::lock_guard<std::mutex> lock(shared::fetchedStationMutex);

        std::copy_if(shared::fetchedStations.begin(),
            shared::fetchedStations.end(), std::back_inserter(rxBar),
            [this](const ns::Station& s) {
                return pClient->GetRxState(s.getFrequencyHz());
            });

        std::copy_if(shared::fetchedStations.begin(),
            shared::fetchedStations.end(), std::back_inserter(txBar),
            [this](const ns::Station& s) {
                return pClient->GetTxState(s.getFrequencyHz());
            });

        std::copy_if(shared::fetchedStations.begin(),
            shared::fetchedStations.end(), std::back_inserter(xcBar),
            [this](const ns::Station& s) {
                return pClient->GetXcState(s.getFrequencyHz());
            });

        jsonMessage["value"]["rx"] = rxBar;
        jsonMessage["value"]["tx"] = txBar;
        jsonMessage["value"]["xc"] = txBar;

        this->broadcastOnWebsocket(jsonMessage.dump());

        return;
    }
};

void SDK::buildRouter()
{
    this->pRouter = std::make_unique<restinio::router::express_router_t<>>();

    this->pRouter->http_get(
        mSDKCallUrl[sdkCall::kTransmitting], [&](auto req, auto /*params*/) {
            return SDK::handleTransmittingSDKCall(req);
        });

    this->pRouter->http_get(mSDKCallUrl[sdkCall::kRx],
        [&](auto req, auto /*params*/) { return this->handleRxSDKCall(req); });

    this->pRouter->http_get(mSDKCallUrl[sdkCall::kTx],
        [&](auto req, auto /*params*/) { return this->handleTxSDKCall(req); });

    this->pRouter->http_get(mSDKCallUrl[sdkCall::kWebSocket],
        [&](auto req, auto /*params*/) { return handleWebSocketSDKCall(req); });

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

restinio::request_handling_status_t SDK::handleTransmittingSDKCall(
    const restinio::request_handle_t& req)
{
    const std::lock_guard<std::mutex> lock(shared::transmittingMutex);
    return req->create_response()
        .set_body(shared::currentlyTransmittingApiData)
        .done();
};

restinio::request_handling_status_t SDK::handleRxSDKCall(
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

restinio::request_handling_status_t SDK::handleTxSDKCall(
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

restinio::request_handling_status_t SDK::handleWebSocketSDKCall(
    const restinio::request_handle_t& req)
{
    if (restinio::http_connection_header_t::upgrade
        != req->header().connection()) {
        return restinio::request_rejected();
    }

    auto wsh = restinio::websocket::basic::upgrade<serverTraits>(*req,
        restinio::websocket::basic::activation_t::immediate,
        [&](auto wsh, auto m) {
            if (restinio::websocket::basic::opcode_t::ping_frame
                == m->opcode()) {
                // Ping-Pong
                auto resp = *m;
                resp.set_opcode(
                    restinio::websocket::basic::opcode_t::pong_frame);
                wsh->send_message(resp);
            } else if (restinio::websocket::basic::opcode_t::
                           connection_close_frame
                == m->opcode()) {
                // Close connection
                this->pWsRegistry.erase(wsh->connection_id());
            }
        });

    // Store websocket connection
    this->pWsRegistry.emplace(wsh->connection_id(), wsh);

    // Upon connection, send the status of frequencies straight away
    {
        std::lock_guard<std::mutex> lock(shared::fetchedStationMutex);
        this->handleAFVEventForWebsocket(
            sdk::types::Event::kFrequencyStateUpdate, std::nullopt,
            std::nullopt);
    }

    return restinio::request_accepted();
};

void SDK::broadcastOnWebsocket(const std::string& data)
{
    restinio::websocket::basic::message_t outgoingMessage;
    outgoingMessage.set_opcode(
        restinio::websocket::basic::opcode_t::text_frame);
    outgoingMessage.set_payload(data);

    for (auto& [id, ws] : this->pWsRegistry) {
        try {
            ws->send_message(outgoingMessage);
        } catch (const std::exception& ex) {
            spdlog::error("Failed to send data to websocket: {}", ex.what());
        }
    }
};
}
