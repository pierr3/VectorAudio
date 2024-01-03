#pragma once

#include "afv-native/atcClientWrapper.h"
#include "shared.h"
#include "util.h"

#include <map>
#include <memory>
#include <restinio/all.hpp>
#include <restinio/common_types.hpp>
#include <restinio/http_headers.hpp>
#include <restinio/request_handler.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

namespace vector_audio {
class SDK {

public:
    explicit SDK(const std::shared_ptr<afv_native::api::atcClient>& clientPtr);

    ~SDK();

    bool start();

    static void loopCleanup(
        const std::vector<std::string>& liveReceivedCallsigns);

private:
    using serverTraits = restinio::traits_t<restinio::asio_timer_manager_t,
        restinio::null_logger_t, restinio::router::express_router_t<>>;
    restinio::running_server_handle_t<serverTraits> pSDKServer;
    std::shared_ptr<afv_native::api::atcClient> pClient;

    enum sdkCall {
        kTransmitting,
        kRx,
        kTx,
    };

    static inline std::map<sdkCall, std::string> mSDKCallUrl = {
        { kTransmitting, "/transmitting" },
        { kRx, "/rx" },
        { kTx, "/tx" },
    };

    void buildServer();

    std::unique_ptr<restinio::router::express_router_t<>> pRouter;

    void buildRouter();

    static restinio::request_handling_status_t transmittingSDKCall(
        const restinio::request_handle_t& req);

    restinio::request_handling_status_t rxSDKCall(
        const restinio::request_handle_t& req);
    restinio::request_handling_status_t txSDKCall(
        const restinio::request_handle_t& req);
};
}
