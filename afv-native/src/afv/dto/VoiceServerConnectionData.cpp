/* afv/dto/VoiceServerConnectionData.cpp
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

#include <nlohmann/json.hpp>
#include "afv-native/afv/dto/VoiceServerConnectionData.h"
#include "afv-native/cryptodto/dto/ChannelConfig.h"

using namespace afv_native::afv::dto;
using json = nlohmann::json;
using ChannelConfig = afv_native::cryptodto::dto::ChannelConfig;

void
afv_native::afv::dto::to_json(json &j, const VoiceServerConnectionData &vsd)
{
    j = json{
        {"addressIpV4",   vsd.AddressIpV4},
        {"addressIpV6",   vsd.AddressIpV6},
        {"channelConfig", vsd.ChannelConfig},
    };
}

void
afv_native::afv::dto::from_json(const json &j, VoiceServerConnectionData &vsd)
{
    j.at("addressIpV4").get_to(vsd.AddressIpV4);
    j.at("addressIpV6").get_to(vsd.AddressIpV6);
    j.at("channelConfig").get_to(vsd.ChannelConfig);
}

VoiceServerConnectionData::VoiceServerConnectionData():
    AddressIpV4(),
    AddressIpV6(),
    ChannelConfig()
{
}

VoiceServerConnectionData::VoiceServerConnectionData(const VoiceServerConnectionData &cpysrc):
    AddressIpV4(cpysrc.AddressIpV4),
    AddressIpV6(cpysrc.AddressIpV6),
    ChannelConfig(cpysrc.ChannelConfig)
{
}

VoiceServerConnectionData::VoiceServerConnectionData(VoiceServerConnectionData &&movesrc) noexcept:
    AddressIpV4(std::move(movesrc.AddressIpV4)),
    AddressIpV6(std::move(movesrc.AddressIpV6)),
    ChannelConfig(std::move(movesrc.ChannelConfig))
{
}
