#include <map>
#include <nlohmann/json.hpp>
#include <string>

namespace ns {

class Airport {
public:
    // Although we have more data available, we don't actually need it, so
    // ignoring for now
    std::string icao;
    int elevation;
    double lat;
    double lon;

    // Store all the loaded airports
    static inline std::map<std::string, Airport> mAll;
};
}
