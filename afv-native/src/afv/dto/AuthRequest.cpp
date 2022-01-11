/* afv/dto/AuthRequest.cpp
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

#include "afv-native/afv/dto/AuthRequest.h"
#include "afv-native/afv/params.h"
#include <nlohmann/json.hpp>

using namespace afv_native::afv::dto;
using namespace std;
using json = nlohmann::json;

AuthRequest::AuthRequest() :
    Username(),
    Password(),
    Client("AFV-Native")
{
}

AuthRequest::AuthRequest(string username, string password, string client) :
    Username(std::move(username)),
    Password(std::move(password)),
    Client(std::move(client))
{
}

AuthRequest::AuthRequest(const AuthRequest& cpysrc) :
    Username(cpysrc.Username),
    Password(cpysrc.Password),
    Client(cpysrc.Client)
{
}

AuthRequest::AuthRequest(AuthRequest&& movesrc) noexcept :
    Username(std::move(movesrc.Username)),
    Password(std::move(movesrc.Password)),
    Client(std::move(movesrc.Client))
{
}

void
afv_native::afv::dto::from_json(const json& j, AuthRequest& ar)
{
    j.at("username").get_to(ar.Username);
    j.at("password").get_to(ar.Password);
    j.at("client").get_to(ar.Client);
}

void
afv_native::afv::dto::to_json(json& j, const AuthRequest& ar)
{
    j = json{
        {"username", ar.Username},
        {"password", ar.Password},
        {"client",   ar.Client},
    };
}