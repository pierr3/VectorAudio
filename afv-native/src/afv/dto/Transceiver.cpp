/* afv/dto/Transceiver.cpp
 *
 * This file is part of AFV-Native.
 *
 * Copyright (c) 2019 Christopher Collins
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include "afv-native/afv/dto/Transceiver.h"
#include <nlohmann/json.hpp>

using namespace afv_native::afv::dto;
using json = nlohmann::json;

Transceiver::Transceiver(uint16_t id, uint32_t freq, double lat, double lon, double msl, double agl):
        ID(id),
        Frequency(freq),
        LatDeg(lat),
        LonDeg(lon),
        HeightMslM(msl),
        HeightAglM(agl)
{
}

void afv_native::afv::dto::from_json(const json &j, Transceiver &ar)
{
    j.at("ID").get_to(ar.ID);
    j.at("Frequency").get_to(ar.Frequency);
    j.at("LatDeg").get_to(ar.LatDeg);
    j.at("LonDeg").get_to(ar.LonDeg);
    j.at("HeightMslM").get_to(ar.HeightMslM);
    j.at("HeightAglM").get_to(ar.HeightAglM);
};

void afv_native::afv::dto::to_json(json &j, const Transceiver &ar)
{
    j = nlohmann::json{
            {"ID",         ar.ID},
            {"Frequency",  ar.Frequency},
            {"LatDeg",     ar.LatDeg},
            {"LonDeg",     ar.LonDeg},
            {"HeightMslM", ar.HeightMslM},
            {"HeightAglM", ar.HeightAglM},
    };
}
