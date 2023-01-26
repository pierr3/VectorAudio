#include <string>
#include <nlohmann/json.hpp>

namespace ns {
    class Airport {
        public:
            // Although we have more data available, we don't actually need it, so ignoring for now
            std::string icao;
            int elevation;
            double lat;
            double lon;

        inline void from_json(const nlohmann::json &j, Airport &ar) {
            j.at("icao").get_to(ar.icao);
            j.at("elevation").get_to(ar.elevation);
            j.at("lat").get_to(ar.lat);
            j.at("lon").get_to(ar.lon);
        };
    };
}
