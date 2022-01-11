//
//  StationTransceiver.cpp
//  afv_native
//
//  Created by Mike Evans on 10/18/20.
//

#include "afv-native/afv/dto/StationTransceiver.h"
#include <nlohmann/json.hpp>

using namespace afv_native::afv::dto;
using json = nlohmann::json;


StationTransceiver::StationTransceiver():
    ID(""),
    Name(""),
    LatDeg(0),
    LonDeg(0),
    HeightMslM(0),
    HeightAglM(0)
{
}

StationTransceiver::StationTransceiver(std::string id, std::string name, double lat, double lon, double msl, double agl):
        ID(id),
        Name(name),
        LatDeg(lat),
        LonDeg(lon),
        HeightMslM(msl),
        HeightAglM(agl)
{
}

void afv_native::afv::dto::from_json(const json &j, StationTransceiver &ar)
{
    j.at("id").get_to(ar.ID);
    j.at("name").get_to(ar.Name);
    j.at("latDeg").get_to(ar.LatDeg);
    j.at("lonDeg").get_to(ar.LonDeg);
    j.at("heightMslM").get_to(ar.HeightMslM);
    j.at("heightAglM").get_to(ar.HeightAglM);
};

void afv_native::afv::dto::to_json(json &j, const StationTransceiver &ar)
{
    j = nlohmann::json{
            {"id",         ar.ID},
            {"name",       ar.Name},
            {"latDeg",     ar.LatDeg},
            {"lonDeg",     ar.LonDeg},
            {"heightMslM", ar.HeightMslM},
            {"heightAglM", ar.HeightAglM},
    };
}

