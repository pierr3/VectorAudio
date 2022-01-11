/* cryptodto/dto/ChannelConfig.cpp
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

#include "afv-native/cryptodto/dto/ChannelConfig.h"

#include <algorithm>
#include <nlohmann/json.hpp>

#include <afv-native/cryptodto/params.h>
#include <afv-native/util/base64.h>

using json = nlohmann::json;
using namespace std;
using namespace afv_native::cryptodto::dto;
using namespace afv_native::cryptodto;
using namespace afv_native::util;

ChannelConfig::ChannelConfig():
    ChannelTag(),
    HmacKey()
{
    memset(AeadTransmitKey, 0, aeadModeKeySize);
    memset(AeadReceiveKey, 0, aeadModeKeySize);
}

ChannelConfig::ChannelConfig(const ChannelConfig &cpysrc):
    ChannelTag(cpysrc.ChannelTag),
    HmacKey(cpysrc.HmacKey)
{
    memcpy(AeadTransmitKey, cpysrc.AeadTransmitKey, aeadModeKeySize);
    memcpy(AeadReceiveKey, cpysrc.AeadReceiveKey, aeadModeKeySize);
}

ChannelConfig::ChannelConfig(ChannelConfig &&movesrc) noexcept:
    ChannelTag(std::move(movesrc.ChannelTag)),
    HmacKey(std::move(movesrc.HmacKey))
{
    memcpy(AeadTransmitKey, movesrc.AeadTransmitKey, aeadModeKeySize);
    memcpy(AeadReceiveKey, movesrc.AeadReceiveKey, aeadModeKeySize);
}

static void
setKey(unsigned char *key, const string &base64_key, size_t outputSize)
{
    size_t input_limit = 4 * ((outputSize + 2) / 3);
    std::string key_copy;
    if (base64_key.length() > input_limit) {
        key_copy = base64_key.substr(0, input_limit);
    } else {
        key_copy = base64_key;
    }

    const size_t key_buffer_len = Base64DecodeLen(key_copy.length());
    std::vector<unsigned char> key_buffer(key_buffer_len);
    size_t final_len = Base64Decode(key_copy, key_buffer.data(), key_buffer_len);
    ::memcpy(key, key_buffer.data(), min(final_len, outputSize));
}

void
afv_native::cryptodto::dto::to_json(json &j, const ChannelConfig &cc)
{
    auto receiveKey = Base64Encode(cc.AeadReceiveKey, aeadModeKeySize);
    auto transmitKey = Base64Encode(cc.AeadTransmitKey, aeadModeKeySize);
    j = json{
        {"channelTag",      cc.ChannelTag},
        {"aeadReceiveKey",  receiveKey},
        {"aeadTransmitKey", transmitKey},
        {"hmacKey",         nullptr},
    };
}

void
afv_native::cryptodto::dto::from_json(const json &j, ChannelConfig &cc)
{
    std::string receiveKeyB64;
    std::string transmitKeyB64;

    j.at("channelTag").get_to(cc.ChannelTag);
    j.at("aeadReceiveKey").get_to(receiveKeyB64);
    j.at("aeadTransmitKey").get_to(transmitKeyB64);
    //j.at("hmacKey").get_to(cc.HmacKey);
    setKey(cc.AeadReceiveKey, receiveKeyB64, aeadModeKeySize);
    setKey(cc.AeadTransmitKey, transmitKeyB64, aeadModeKeySize);
};
