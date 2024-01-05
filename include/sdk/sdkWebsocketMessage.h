#pragma once
#include <iostream>
#include <map>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

namespace vector_audio::sdk::types {
enum class WebsocketMessageType {
    kRxBegin,
    kRxEnd,
    kFrequencyStateUpdate
};

const std::map<WebsocketMessageType, std::string> kWebsocketMessageTypeMap {
    { WebsocketMessageType::kRxBegin, "kRxBegin" },
    { WebsocketMessageType::kRxEnd, "kRxEnd" },
    { WebsocketMessageType::kFrequencyStateUpdate, "kFrequenciesUpdate" }
};

class WebsocketMessage {
public:
    std::string type;
    nlohmann::json value;

    explicit WebsocketMessage(std::string type)
        : type(std::move(type))
        , value({})
    {
    }

    static WebsocketMessage buildMessage(WebsocketMessageType messageType)
    {
        const std::string& typeString
            = kWebsocketMessageTypeMap.at(messageType);
        return WebsocketMessage(typeString);
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(WebsocketMessage, type, value);
};

} // namespace vector_audio::sdk

// Example of kRxBegin message:
// @type the type of the message
// @value the callsign of the station (pilot or ATC) who started transmitting the radio, and
// the frequency which is being transmitted on
// JSON: {"type": "kRxBegin", "value": {"callsign": "AFR001",
// "pFrequencyHz": 123000000}}

// Example of kRxEnd message:
// @type the type of the message
// @value the callsign of the station (pilot or ATC) who stopped transmitting the radio, and
// the frequency which was being transmitted on
// JSON: {"type": "kRxEnd", "value": {"callsign": "AFR001",
// "pFrequencyHz": 123000000}}

//
// Example of kFrequencyStateUpdate message:
// @type the type of the message
// @value the frequency state update information including rx, tx, and xc
// stations
// JSON: {"type": "kFrequencyStateUpdate", "value": {"rx":
// [{"pFrequencyHz": 118775000, "pCallsign": "EDDF_S_TWR"}], "tx": [{"pFrequencyHz": 119775000, "pCallsign": "EDDF_S_TWR"}], "xc":
// [{"pFrequencyHz": 121500000, "pCallsign": "EDDF_S_TWR"}]}}
