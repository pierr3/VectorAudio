#include "sdk/sdk.h"

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
    shared::currentlyTransmittingApiData
        = absl::StrJoin(liveReceivedCallsigns, ",");

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

        // std::lock_guard<std::mutex> lock(shared::fetchedStationMutex);
        // Lock needed outside of this function due to it being called somewhere
        // where the mutex is already locked

        std::vector<ns::Station> rxBar;
        for (const auto& s : shared::fetchedStations) {
            if (pClient->GetRxState(s.getFrequencyHz())) {
                rxBar.push_back(s);
            }
        }

        std::vector<ns::Station> txBar;
        for (const auto& s : shared::fetchedStations) {
            if (pClient->GetTxState(s.getFrequencyHz())) {
                txBar.push_back(s);
            }
        }

        std::vector<ns::Station> xcBar;
        for (const auto& s : shared::fetchedStations) {
            if (pClient->GetXcState(s.getFrequencyHz())) {
                xcBar.push_back(s);
            }
        }

        jsonMessage["value"]["rx"] = std::move(rxBar);
        jsonMessage["value"]["tx"] = std::move(txBar);
        jsonMessage["value"]["xc"] = std::move(xcBar);

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
    if (!pClient->IsVoiceConnected()) {
        return req->create_response().set_body("").done();
    }

    std::lock_guard<std::mutex> lock(shared::fetchedStationMutex);

    std::string out;
    for (const auto& f : shared::fetchedStations) {
        if (!pClient->GetRxState(f.getFrequencyHz())) {
            continue;
        }
        out += f.getCallsign() + ":" + f.getHumanFrequency() + ",";
    }

    if (!out.empty()) {
        if (out.back() == ',') {
            out.pop_back();
        }
    }

    return req->create_response().set_body(out).done();
};

restinio::request_handling_status_t SDK::handleTxSDKCall(
    const restinio::request_handle_t& req)
{
    if (!pClient->IsVoiceConnected()) {
        return req->create_response().set_body("").done();
    }

    std::lock_guard<std::mutex> lock(shared::fetchedStationMutex);

    std::string out;
    for (const auto& f : shared::fetchedStations) {
        if (!pClient->GetTxState(f.getFrequencyHz())) {
            continue;
        }
        out += f.getCallsign() + ":" + f.getHumanFrequency() + ",";
    }

    if (!out.empty()) {
        if (out.back() == ',') {
            out.pop_back();
        }
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
