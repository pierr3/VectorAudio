//
//  StationTransceiver.hpp
//  afv_native
//
//  Created by Mike Evans on 10/18/20.
//

#ifndef StationTransceiver_h
#define StationTransceiver_h


#include <cstdint>
#include <msgpack.hpp>
#include <nlohmann/json.hpp>

namespace afv_native {
    namespace afv {
        namespace dto {
            class StationTransceiver {
            public:
                std::string ID;
                std::string Name;
                double LatDeg;
                double LonDeg;
                double HeightMslM;
                double HeightAglM;

                StationTransceiver();
                StationTransceiver(std::string id, std::string name, double lat, double lon, double msl, double agl);

                MSGPACK_DEFINE_ARRAY(ID, Name, LatDeg, LonDeg, HeightMslM, HeightAglM);
            };

            void from_json(const nlohmann::json &j, StationTransceiver &ar);
            void to_json(nlohmann::json &j, const StationTransceiver &ar);
        }
    }
}


#endif /* StationTransceiver_h */
