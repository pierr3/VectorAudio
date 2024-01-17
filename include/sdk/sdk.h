#pragma once

#include "absl/strings/str_join.h"
#include "afv-native/atcClientWrapper.h"
#include "afv-native/event.h"
#include "ns/station.h"
#include "sdkWebsocketMessage.h"
#include "shared.h"
#include "util.h"

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <restinio/all.hpp>
#include <restinio/common_types.hpp>
#include <restinio/http_headers.hpp>
#include <restinio/request_handler.hpp>
#include <restinio/router/express.hpp>
#include <restinio/traits.hpp>
#include <restinio/websocket/message.hpp>
#include <restinio/websocket/websocket.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

namespace vector_audio {

using sdk::types::WebsocketMessage;
using sdk::types::WebsocketMessageType;

namespace sdk::types {
    enum Event {
        kRxBegin,
        kRxEnd,
        kFrequencyStateUpdate,
    };
}

class SDK {

public:
    explicit SDK(const std::shared_ptr<afv_native::api::atcClient>& clientPtr);
    ~SDK();

    bool start();

    /**
     * Handles an AFV event for the websocket.
     *
     * @param event The AFV event to handle.
     * @param data Optional data associated with the event.
     */
    void handleAFVEventForWebsocket(sdk::types::Event event,
        const std::optional<std::string>& callsign,
        const std::optional<int>& frequencyHz);

    static void loopCleanup(
        const std::vector<std::string>& liveReceivedCallsigns);

private:
    using serverTraits = restinio::traits_t<restinio::asio_timer_manager_t,
        restinio::null_logger_t, restinio::router::express_router_t<>>;

    restinio::running_server_handle_t<serverTraits> pSDKServer;
    std::shared_ptr<afv_native::api::atcClient> pClient;

    using ws_registry_t
        = std::map<std::uint64_t, restinio::websocket::basic::ws_handle_t>;

    ws_registry_t pWsRegistry;

    enum sdkCall {
        kTransmitting,
        kRx,
        kTx,
        kWebSocket,
    };

    static inline std::map<sdkCall, std::string> mSDKCallUrl
        = { { kTransmitting, "/transmitting" }, { kRx, "/rx" }, { kTx, "/tx" },
              { kWebSocket, "/ws" } };

    /**
     * @brief Broadcasts data on the websocket.
     *
     * This function sends the provided data on the websocket connection.
     *
     * @param data The data to be broadcasted in JSON format.
     */
    void broadcastOnWebsocket(const std::string& data);

    /**
     * @brief Builds the server.
     *
     * This function is responsible for building the server.
     * It performs the necessary initialization and setup tasks
     * to create a functioning server.
     */
    void buildServer();

    std::unique_ptr<restinio::router::express_router_t<>> pRouter;

    /**
     * @brief Builds the router.
     */
    void buildRouter();

    /**
     * Handles the SDK call for transmitting data.
     *
     * @param req The request handle.
     * @return The status of request handling.
     */
    static restinio::request_handling_status_t handleTransmittingSDKCall(
        const restinio::request_handle_t& req);

    /**
     * Handles the SDK call received in the request.
     *
     * @param req The request handle.
     * @return The status of the request handling.
     */
    restinio::request_handling_status_t handleRxSDKCall(
        const restinio::request_handle_t& req);
    /**
     * Handles the SDK call.
     *
     * @param req The request handle.
     * @return The request handling status.
     */
    restinio::request_handling_status_t handleTxSDKCall(
        const restinio::request_handle_t& req);

    /**
     * Handles a WebSocket SDK call.
     *
     * @param req The request handle.
     * @return The status of the request handling.
     */
    restinio::request_handling_status_t handleWebSocketSDKCall(
        const restinio::request_handle_t& req);
};
}
