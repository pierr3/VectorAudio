//
//  CrossCoupleGroup.cpp
//  afv_native
//
//  Created by Mike Evans on 10/17/20.
//

#include "afv-native/afv/dto/CrossCoupleGroup.h"

#include <nlohmann/json.hpp>

using namespace afv_native::afv::dto;
using json = nlohmann::json;

CrossCoupleGroup::CrossCoupleGroup(uint16_t id, std::vector<uint16_t>TransceiverIDs):
        ID(id),
        TransceiverIDs(TransceiverIDs)
{
}

void afv_native::afv::dto::from_json(const json &j, CrossCoupleGroup &ar)
{
    j.at("ID").get_to(ar.ID);
    j.at("TransceiverIDs").get_to(ar.TransceiverIDs);
};

void afv_native::afv::dto::to_json(json &j, const CrossCoupleGroup &ar)
{
    j = nlohmann::json{
            {"ID",         ar.ID},
            {"TransceiverIDs",  ar.TransceiverIDs},
    };
}


