/* cryptodto/UDPChannel.cpp
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

#include "afv-native/cryptodto/UDPChannel.h"
#include "afv-native/cryptodto/dto/ChannelConfig.h"

#include <cerrno>
#include <event2/util.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <ws2ipdef.h>
#include <afv-native/cryptodto/dto/ChannelConfig.h>

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <unistd.h>
#endif

#include "afv-native/Log.h"

using namespace afv_native::cryptodto;
using namespace std;

UDPChannel::UDPChannel(struct event_base* evBase, int receiveSequenceHistorySize) :
    Channel(),
    mAddress(),
    mDatagramRxBuffer(nullptr),
    mUDPSocket(-1),
    mEvBase(evBase),
    mSocketEvent(nullptr),
    mTxSequence(0),
    receiveSequence(0, receiveSequenceHistorySize),
    mAcceptableCiphers(1U << cryptodto::CryptoDtoMode::CryptoModeChaCha20Poly1305),
    mDtoHandlers(),
    mLastErrno(0)
{
    mDatagramRxBuffer = new unsigned char[maxPermittedDatagramSize];
}

UDPChannel::~UDPChannel()
{
    close();
    delete[] mDatagramRxBuffer;
    mDatagramRxBuffer = nullptr;
}

void UDPChannel::registerDtoHandler(
    const string& dtoName,
    std::function<void(const unsigned char* data, size_t len)> callback)
{
    mDtoHandlers[dtoName] = callback;
}

void UDPChannel::evReadCallback(evutil_socket_t fd, short events, void* arg)
{
    auto* channel = reinterpret_cast<UDPChannel*>(arg);
    channel->readCallback();
}

void UDPChannel::readCallback()
{
    int dgSize = ::recv(
        mUDPSocket, reinterpret_cast<char*>(mDatagramRxBuffer), maxPermittedDatagramSize, 0);
    if (dgSize < 0)
    {
        mLastErrno = evutil_socket_geterror(mUDPSocket);
        LOG("udpchannel:readCallback", "recv error: %s", evutil_socket_error_to_string(mLastErrno));
        return;
    }
    if (dgSize > maxPermittedDatagramSize)
    {
        LOG("udpchannel:readCallback",
            "recv'd datagram %d bytes, exceeding configured maximum of %d",
            dgSize,
            maxPermittedDatagramSize);
        return;
    }
    std::string channelTag, dtoName;
    sequence_t seq;
    msgpack::sbuffer dtoBuf;
    CryptoDtoMode cipherMode;

    if (!Decapsulate(mDatagramRxBuffer, dgSize, channelTag, seq, cipherMode, dtoName, dtoBuf))
    {
        LOG("udpchannel:readCallback", "recv'd invalid cryptodto frame.  Discarding");
        return;
    }
    if (!RxModeEnabled(cipherMode))
    {
        LOG("udpchannel:readCallback", "got frame encrypted with undesired mode");
        return;
    }
    if (channelTag != ChannelTag)
    {
        LOG("udpchannel:readCallback", "recv'd with invalid Tag.  Discarding");
        return;
    }
    auto rxOk = receiveSequence.Received(seq);
    switch (rxOk)
    {
        case ReceiveOutcome::Before:
            LOG("udpchannel:readCallback", "recv'd duplicate sequence %d.  Discarding.", seq);
            return;
        case ReceiveOutcome::OK:
            break;
        case ReceiveOutcome::Overflow:
            break;
    }
    // validate that the packet has a valid payload.
    if (dtoBuf.size() < 2)
    {
        LOG("udpchannel:readCallback", "internal dto had bad length (too short)");
        return;
    }
    uint16_t dtoSize;
    ::memcpy(&dtoSize, dtoBuf.data(), 2);
    if (dtoSize != dtoBuf.size() - 2)
    {
        LOG("udpchannel:readCallback", "internal dto had bad length (length encoded mismatched datagram size)");
        return;
    }
    auto dtoIter = mDtoHandlers.find(dtoName);
    if (dtoIter == mDtoHandlers.end())
    {
        LOG("udpchannel:readCallback", "no handler for packet-type %s", dtoName.c_str());
        return;
    }
    else
    {
        if (dtoBuf.size() == 2)
        {
            dtoIter->second(nullptr, 0);
        }
        else
        {
            dtoIter->second(reinterpret_cast<const unsigned char*>(dtoBuf.data()) + 2, dtoBuf.size() - 2);
        }
    }
}

bool UDPChannel::open()
{
    if (mAddress.empty())
    {
        LOG("udpchannel", "tried to open without address set");
        return false;
    }
    struct sockaddr_storage saddr;
    int saddr_len = sizeof(saddr);

    if (!evutil_parse_sockaddr_port(mAddress.c_str(), reinterpret_cast<struct sockaddr*>(&saddr), &saddr_len))
    {
        mUDPSocket = ::socket(saddr.ss_family, SOCK_DGRAM, 0);
        if (mUDPSocket < 0)
        {
            mLastErrno = EVUTIL_SOCKET_ERROR();
            LOG("udpchannel", "Couldn't create UDP socket: %s", evutil_socket_error_to_string(errno));
            close();
            return false;
        }
        evutil_make_socket_nonblocking(mUDPSocket);

        if (saddr.ss_family == AF_INET6)
        {
            struct sockaddr_in6 baddr = { AF_INET6, 0, 0, IN6ADDR_ANY_INIT, 0, };
            if (::bind(mUDPSocket, reinterpret_cast<struct sockaddr*>(&baddr), sizeof(baddr)))
            {
                mLastErrno = evutil_socket_geterror(mUDPSocket);
                LOG("udpchannel", "couldn't bind IPv6 port: %s", evutil_socket_error_to_string(errno));
                close();
                return false;
            }
        }
        else
        {
            // IPV4.
            struct sockaddr_in baddr = { AF_INET, 0, INADDR_ANY, };
            if (::bind(mUDPSocket, reinterpret_cast<struct sockaddr*>(&baddr), sizeof(baddr)))
            {
                mLastErrno = evutil_socket_geterror(mUDPSocket);
                LOG("udpchannel", "couldn't bind IPv4 port: %s", evutil_socket_error_to_string(errno));
                close();
                return false;
            }
        }
        if (::connect(mUDPSocket, reinterpret_cast<struct sockaddr*>(&saddr), saddr_len))
        {
            mLastErrno = evutil_socket_geterror(mUDPSocket);
            LOG("udpchannel", "couldn't connect to endpoint address \"%s\": %s", mAddress.c_str(), evutil_socket_error_to_string(errno));
            close();
            return false;
        }

        // bind up the libevent handling
        mSocketEvent = event_new(mEvBase, mUDPSocket, EV_READ | EV_PERSIST, UDPChannel::evReadCallback, this);
        event_add(mSocketEvent, nullptr);
        return true;
    }
    return false;
}

void UDPChannel::close()
{
    if (mSocketEvent != nullptr)
    {
        event_del(mSocketEvent);
        event_free(mSocketEvent);
        mSocketEvent = nullptr;
    }
    if (mUDPSocket >= 0)
    {
        #ifdef WIN32
        ::closesocket(mUDPSocket);
        #else
        ::close(mUDPSocket);
        #endif
        mUDPSocket = -1;
    }
    receiveSequence.reset();
}

void UDPChannel::setAddress(const std::string& address)
{
    mAddress = address;
}

bool UDPChannel::isOpen() const
{
    return (mUDPSocket >= 0);
}

void UDPChannel::enableRxMode(CryptoDtoMode mode)
{
    if (mode >= CryptoModeLast)
    {
        return;
    }
    unsigned int mask = 1U << mode;

    mAcceptableCiphers |= mask;
}

void UDPChannel::disableRxMode(CryptoDtoMode mode)
{
    if (mode >= CryptoModeLast)
    {
        return;
    }
    unsigned int mask = 1U << mode;

    mAcceptableCiphers &= ~mask;
}

bool UDPChannel::RxModeEnabled(CryptoDtoMode mode) const
{
    if (mode >= CryptoModeLast)
    {
        return false;
    }
    unsigned int mask = 1U << mode;
    return mask == (mAcceptableCiphers & mask);
}

void UDPChannel::unregisterDtoHandler(const std::string& dtoName)
{
    mDtoHandlers.erase(dtoName);
}

int UDPChannel::getLastErrno() const
{
    return mLastErrno;
}

void UDPChannel::setChannelConfig(const dto::ChannelConfig& config)
{
    // if the channel keys change, we need to reset our rx expected sequence as the cipher
    // has probably restarted.  We do not need to reset tx since the other end will deal.
    if (::memcmp(aeadReceiveKey, config.AeadReceiveKey, aeadModeKeySize) != 0)
    {
        receiveSequence.reset();
    }
    Channel::setChannelConfig(config);
}
