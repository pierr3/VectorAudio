//
//  CrossCoupleGroup.hpp
//  afv_native
//
//  Created by Mike Evans on 10/17/20.
//

#ifndef CrossCoupleGroup_h
#define CrossCoupleGroup_h

#include <cstdint>
#include <msgpack.hpp>
#include <nlohmann/json.hpp>
#include "afv-native/afv/dto/Transceiver.h"

namespace afv_native {
    namespace afv {
        namespace dto {
            class CrossCoupleGroup {
            public:
                uint16_t ID;
                std::vector<uint16_t> TransceiverIDs;

                CrossCoupleGroup(uint16_t id, std::vector<uint16_t> TransceiverIDs);

               
            };

            void from_json(const nlohmann::json &j, CrossCoupleGroup &ar);
            void to_json(nlohmann::json &j, const CrossCoupleGroup &ar);
        }
    }
}

#endif /* CrossCoupleGroup_h */
