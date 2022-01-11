/* afv/APISession.cpp
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

#include "afv-native/afv/APISession.h"

#include <string>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <jwt/jwt.hpp>

#include "afv-native/afv/VoiceSession.h"
#include "afv-native/afv/params.h"
#include "afv-native/http/RESTRequest.h"
#include "afv-native/afv/dto/AuthRequest.h"
#include "afv-native/afv/dto/PostCallsignResponse.h"

using namespace ::afv_native::afv;
using namespace ::afv_native;
using json = nlohmann::json;

APISession::APISession(event_base* evBase, http::TransferManager& tm, std::string baseUrl, std::string clientName) :
    StateCallback(),
    AliasUpdateCallback(),
    mEvBase(evBase),
    mTransferManager(tm),
    mBaseURL(std::move(baseUrl)),
    mUsername(),
    mPassword(),
    mClientName(std::move(clientName)),
    mBearerToken(),
    mAuthenticationRequest(mBaseURL + "/api/v1/auth", http::Method::POST, json()),
    mRefreshTokenTimer(mEvBase, std::bind(&APISession::Connect, this)),
    mLastError(APISessionError::NoError),
    mStationAliasRequest(mBaseURL + "/api/v1/stations/aliased", http::Method::GET, nullptr),
    mState(APISessionState::Disconnected)
{
}

void
APISession::Connect()
{
    dto::AuthRequest ar(mUsername, mPassword, mClientName);

    /* start the authentication request */
    mAuthenticationRequest.reset();
    mAuthenticationRequest.setUrl(mBaseURL + "/api/v1/auth");
    mAuthenticationRequest.setRequestBody(ar);
    mAuthenticationRequest.setCompletionCallback(
        [this](http::Request* req, bool success)
    {
        auto restreq = dynamic_cast<http::RESTRequest*>(req);
        assert(restreq != nullptr); // shouldn't be possible
        this->_authenticationCallback(restreq, success);
    });
    mAuthenticationRequest.shareState(mTransferManager);
    mAuthenticationRequest.doAsync(mTransferManager);
    /* update our internal state */
    switch (mState)
    {
        case APISessionState::Disconnected:
            setState(APISessionState::Connecting);
            break;
        case APISessionState::Running:
            setState(APISessionState::Reconnecting);
            break;
        default:
            break;
    }
}

void
afv::APISession::Disconnect()
{
    mRefreshTokenTimer.disable();
    mBearerToken = "";
    mAuthenticationRequest.reset();
    setState(APISessionState::Disconnected);
}

const std::string&
APISession::getUsername() const
{
    return mUsername;
}

void
APISession::setUsername(const std::string& username)
{
    mUsername = username;
}

void
APISession::setPassword(const std::string& password)
{
    mPassword = password;
}


void
APISession::_authenticationCallback(http::RESTRequest* req, bool success)
{
    if (success && req->getStatusCode() == 200)
    {
        mBearerToken = req->getResponseBody();
        if (mBearerToken.empty())
        {
            LOG("APISession", "No Token Received");
            mBearerToken = "";
            raiseError(APISessionError::InvalidAuthToken);
            return;
        }
        try
        {
            using namespace jwt::params;
            std::error_code ec;
            auto dec_token = jwt::decode(mBearerToken, algorithms({ "none" }), ec, verify(false));
            if (ec)
            {
                LOG("APISession", "couldn't parse bearer token: %s", ec.message().c_str());
                mBearerToken = "";
                raiseError(APISessionError::InvalidAuthToken);
                return;
            }
            else
            {
                if (dec_token.payload().has_claim("exp"))
                {
                    const time_t expiry = dec_token.payload().get_claim_value<uint64_t>("exp");
                    const int timeRemaining = expiry - ::time(nullptr);
                    if (timeRemaining <= 60)
                    {
                        //FIXME: report error upstream.
                        LOG("APISession", "token TTL (%d) is <= 60s.  Please check your system clock.", timeRemaining);
                        mBearerToken = "";
                        raiseError(APISessionError::AuthTokenExpiryTimeInPast);
                        return;
                    }
                    LOG("APISession", "API Token Expires in %d seconds", expiry - time(nullptr));
                    // refresh 1 minute before token expiry.
                    mRefreshTokenTimer.enable((timeRemaining - 60) * 1000);
                }
                else
                {
                    LOG("APISession", "no expiry claim - assuming 1 hour.", ec.message().c_str());
                    mRefreshTokenTimer.enable(59 * 60 * 1000); // refresh in 59 minutes.
                }
            }
        }
        catch (const std::exception& e)
        {
            // if we failed in here, carp about it, and use 6 hours.
            LOG("APISession", "Couldn't parse Bearer Token to get expiry time: %s", e.what());
            mBearerToken = "";
            raiseError(APISessionError::InvalidAuthToken);
            return;
        }
        setState(APISessionState::Running);
    }
    else
    {
        // This is a failure during auth, which is grounds to handle it as
        // if it were an immediate disconnect.
        mBearerToken = "";
        if (!success)
        {
            LOG("APISession", "curl internal error during login: %s", req->getCurlError().c_str());
            raiseError(APISessionError::ConnectionError);
        }
        else
        {
            LOG("APISession", "got error from API server: Response Code %d", req->getStatusCode());
            switch (req->getStatusCode())
            {
                case 400:
                    raiseError(APISessionError::BadRequestOrClientIncompatible);
                    break;
                case 401:
                    raiseError(APISessionError::BadPassword);
                    break;
                case 403:
                    raiseError(APISessionError::RejectedCredentials);
                    break;
                default:
                    raiseError(APISessionError::OtherRequestError);
                    break;
            }
        }
        setState(APISessionState::Disconnected);
    }
    // cleanup and remove
    mAuthenticationRequest.reset();
}

void
APISession::setAuthenticationFor(http::Request& r)
{
    assert(!mBearerToken.empty());
    r.setHeader("Authorization", "Bearer " + mBearerToken);
}

http::TransferManager&
APISession::getTransferManager() const
{
    return mTransferManager;
}

struct event_base*
    APISession::getEventBase() const
{
    return mEvBase;
}

APISessionState
APISession::getState() const
{
    return mState;
}

const std::string&
APISession::getBaseUrl() const
{
    return mBaseURL;
}

void APISession::setBaseUrl(std::string newUrl)
{
    mBaseURL = std::move(newUrl);
}

void APISession::setState(APISessionState newState)
{
    if (newState != mState)
    {
        mState = newState;
        StateCallback.invokeAll(mState);
    }
}

void APISession::raiseError(APISessionError error)
{
    mState = APISessionState::Disconnected;
    mLastError = error;
    StateCallback.invokeAll(APISessionState::Error);
}

APISessionError APISession::getLastError() const
{
    return mLastError;
}

void APISession::updateStationAliases()
{
    /* start the authentication request */
    mStationAliasRequest.reset();
    mStationAliasRequest.setUrl(mBaseURL + "/api/v1/stations/aliased");
    setAuthenticationFor(mStationAliasRequest);
    mStationAliasRequest.setCompletionCallback(
        [this](http::Request* req, bool success)
    {
        auto restreq = dynamic_cast<http::RESTRequest*>(req);
        assert(restreq != nullptr); // shouldn't be possible
        this->_stationsCallback(restreq, success);
    });
    mStationAliasRequest.shareState(mTransferManager);
    mStationAliasRequest.doAsync(mTransferManager);
}

void APISession::_stationsCallback(http::RESTRequest* req, bool success)
{
    if (success && req->getStatusCode() == 200)
    {
        auto jsReturn = req->getResponse();

        if (!jsReturn.is_array())
        {
            LOG("APISession", "station data returned wasn't an array.  Ignoring.");
        }
        else
        {
            mAliasedStations.clear();
            for (const auto& sJson : jsReturn)
            {
                dto::Station s;
                try
                {
                    sJson.get_to(s);
                    mAliasedStations.emplace_back(s);
                }
                catch (nlohmann::json::exception& e)
                {
                    LOG("APISession", "couldn't decode station alias: %s", e.what());
                }
            }
            LOG("APISession", "got %d station aliases.", mAliasedStations.size());
            AliasUpdateCallback.invokeAll();
        }
    }
    else
    {
        if (!success)
        {
            LOG("APISession", "curl internal error during alias retrieval: %s", req->getCurlError().c_str());
            // raiseError(APISessionError::ConnectionError);
        }
        else
        {
            LOG("APISession", "got error from API server getting aliases: Response Code %d", req->getStatusCode());
        }
    }
    // cleanup and remove
    mStationAliasRequest.reset();
}

std::vector<dto::Station> APISession::getStationAliases() const
{
    return mAliasedStations;
}