/* cryptodto/Channel.cpp
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

#include "afv-native/cryptodto/Channel.h"

#include <ctime>
#include <cstring>
#include <string>
#include <openssl/rand.h>

#include "afv-native/cryptodto/dto/Header.h"
#include "afv-native/cryptodto/dto/ChannelConfig.h"

using namespace afv_native::cryptodto;
using namespace std;

Channel::Channel():
        ChannelTag()
{
    make_aead_key(aeadTransmitKey);
    make_aead_key(aeadReceiveKey);
}

void Channel::make_aead_key(unsigned char keyBuffer[])
{
    RAND_priv_bytes(keyBuffer, aeadModeKeySize);
}

void Channel::makeChaCha20Poly1305Nonce(uint64_t sequence, unsigned char *nonceBuffer)
{
    ::memset(nonceBuffer, 0, aeadModeIVSize);
    //NOTE: LE systems only.
    ::memcpy(nonceBuffer + 4, &sequence, sizeof(sequence));
}

size_t Channel::encryptChaCha20Poly1305(
        unsigned char *cipherOut,
        const unsigned char *plainIn,
        size_t plainLen,
        const dto::Header &header,
        const unsigned char *aadIn,
        size_t aadLen)
{
    size_t cipherLen = 0;
    int enc_len = 0;
    unsigned char nonce[aeadModeIVSize];

    makeChaCha20Poly1305Nonce(header.Sequence, nonce);

    auto *cipher_context = EVP_CIPHER_CTX_new();
    //per EVP_EncryptInit, use null keys, then set the keys later with type null.
    if (!EVP_EncryptInit_ex(cipher_context, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr)) {
        goto abort;
    }
    if (!EVP_CIPHER_CTX_ctrl(cipher_context, EVP_CTRL_AEAD_SET_IVLEN, aeadModeIVSize, nullptr)) {
        goto abort;
    }
    if (!EVP_EncryptInit_ex(cipher_context, nullptr, nullptr, aeadTransmitKey, nonce)) {
        goto abort;
    }
    if (aadLen > 0) {
        if (!EVP_EncryptUpdate(cipher_context, nullptr, &enc_len, aadIn, aadLen)) {
            goto abort;
        }
    }
    if (!EVP_EncryptUpdate(cipher_context, cipherOut + cipherLen, &enc_len, plainIn, plainLen)) {
        goto abort;
    }
    cipherLen += enc_len;
    if (!EVP_EncryptFinal_ex(cipher_context, cipherOut + cipherLen, &enc_len)) {
        goto abort;
    }
    cipherLen += enc_len;
    // append the tag.
    if (!EVP_CIPHER_CTX_ctrl(cipher_context, EVP_CTRL_AEAD_GET_TAG, aeadModeTagSize, cipherOut + cipherLen)) {
        goto abort;
    }

    cipherLen += aeadModeTagSize;

    EVP_CIPHER_CTX_free(cipher_context);

    return cipherLen;
    abort:
    EVP_CIPHER_CTX_free(cipher_context);
    return 0;
}

size_t Channel::decryptChaCha20Poly1305(
        unsigned char *bodyOut,
        const unsigned char *cipherIn,
        size_t cipherLen,
        const dto::Header &header,
        const unsigned char *aadIn,
        size_t aadLen)
{
    // make sure the message is long enough
    if (cipherLen < aeadModeTagSize) {
        return 0;
    }

    size_t bodyLen = 0;
    unsigned char nonce[aeadModeIVSize];
    int dec_len = 0;
    auto *cipher_context = EVP_CIPHER_CTX_new();

    makeChaCha20Poly1305Nonce(header.Sequence, nonce);
    if (!EVP_DecryptInit_ex(cipher_context, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr)) {
        goto abort;
    }
    if (!EVP_CIPHER_CTX_ctrl(cipher_context, EVP_CTRL_AEAD_SET_IVLEN, aeadModeIVSize, nullptr)) {
        goto abort;
    }
    if (!EVP_DecryptInit_ex(cipher_context, nullptr, nullptr, aeadReceiveKey, nonce)) {
        goto abort;
    }
    if (!EVP_CIPHER_CTX_ctrl(
            cipher_context, EVP_CTRL_AEAD_SET_TAG, aeadModeTagSize,
            (void *) (cipherIn + (cipherLen - aeadModeTagSize)))) {
        goto abort;
    };
    if (aadLen > 0) {
        if (!EVP_DecryptUpdate(cipher_context, nullptr, &dec_len, aadIn, aadLen)) {
            goto abort;
        }
        dec_len = 0;
    }
    if (!EVP_DecryptUpdate(cipher_context, bodyOut, &dec_len, cipherIn, cipherLen - aeadModeTagSize)) {
        goto abort;
    }
    bodyLen += dec_len;
    dec_len = 0;
    if (!EVP_DecryptFinal_ex(cipher_context, bodyOut + bodyLen, &dec_len)) {
        goto abort;
    }
    bodyLen += dec_len;
    EVP_CIPHER_CTX_free(cipher_context);

    return bodyLen;
    abort:
    EVP_CIPHER_CTX_free(cipher_context);
    return 0;
}

bool Channel::Decapsulate(
        const unsigned char *cipherTextIn,
        size_t cipherTextLen,
        std::string &channelTag,
        sequence_t &sequence,
        CryptoDtoMode &modeOut,
        std::string &dtoNameOut,
        msgpack::sbuffer &dtoOut)
{
    size_t offset = 2;
    if (cipherTextLen < 2) {
        return false;
    }
    uint16_t headerSize = 0;
    ::memcpy(&headerSize, cipherTextIn, sizeof(headerSize));

    // minimum bounds for the full message is the header size + header + dtonamesize + one byte for the dtoname.
    if (cipherTextLen <= (2 + headerSize + 3)) {
        return false;
    }

    auto headerObjHdl = msgpack::unpack(reinterpret_cast<const char *>(cipherTextIn) + offset, headerSize);
    dto::Header header;
    try {
        headerObjHdl.get().convert(header);
    } catch (const msgpack::type_error &) {
        return false;
    }
    offset += headerSize;

    modeOut = static_cast<cryptodto::CryptoDtoMode>(header.Mode);

    const size_t bodySize = cipherTextLen - offset;
    std::vector<unsigned char> bodyOut(bodySize);
    size_t bodyLen = 0;
    switch (header.Mode) {
    case CryptoModeNone:
        ::memcpy(bodyOut.data(), cipherTextIn + offset, bodySize);
        bodyLen = bodySize;
        break;
    case CryptoModeChaCha20Poly1305:
        // make sure the message is long enough
        if (bodySize <= 16) {
            return false;
        }
        bodyLen = decryptChaCha20Poly1305(bodyOut.data(), cipherTextIn + offset, bodySize, header, cipherTextIn, offset);
        if (bodyLen == 0) {
            return false;
        }
        break;
    default:
        return false;
    }

    // now, extract the DTO name.
    uint16_t nameSize;
    ::memcpy(&nameSize, bodyOut.data(), 2);
    if (nameSize > bodyLen) {
        return false;
    }
    dtoNameOut = std::move(string(reinterpret_cast<char *>(bodyOut.data()) + 2, nameSize));
    dtoOut.write(reinterpret_cast<char *>(bodyOut.data()) + 2 + nameSize, bodyLen - 2 - nameSize);

    sequence = header.Sequence;
    channelTag = std::move(header.ChannelTag);
    return true;
}

size_t Channel::Encapsulate(
        const unsigned char *plainTextBuf,
        size_t plainTextLen,
        sequence_t sequence,
        CryptoDtoMode mode,
        unsigned char *cipherTextBufOut,
        size_t cipherTextLen)
{
    size_t offset = 0;
    uint16_t nLen;
    int enc_len;

    msgpack::sbuffer headerBuf;

    // assemble the header and pack it.
    dto::Header myHeader(ChannelTag, sequence, mode);
    msgpack::pack(headerBuf, myHeader);

    if (headerBuf.size() > UINT16_MAX) {
        return 0;
    }

    // now, copy out the header.
    if (cipherTextLen < headerBuf.size() + 2) {
        return 0;
    }
    nLen = headerBuf.size();
    ::memcpy(cipherTextBufOut, &nLen, 2);
    offset += 2;
    ::memcpy(cipherTextBufOut + offset, headerBuf.data(), headerBuf.size());
    offset += headerBuf.size();

    assert(offset == (headerBuf.size() + 2));

    switch (mode) {
    case CryptoModeChaCha20Poly1305:
        if (cipherTextLen < (offset + plainTextLen + 16)) {
            return 0;
        }
        enc_len = encryptChaCha20Poly1305(
                cipherTextBufOut + offset, plainTextBuf, plainTextLen, myHeader, cipherTextBufOut, offset);
        if (0 == enc_len) {
            return 0;
        }
        offset += enc_len;
        assert(offset == (headerBuf.size() + 2 + plainTextLen + 16));
        break;
    case CryptoModeNone:
        if (cipherTextLen < (offset + plainTextLen)) {
            return 0;
        }
        ::memcpy(cipherTextBufOut + offset, plainTextBuf, plainTextLen);
        offset += plainTextLen;
        break;
    default:
        return 0;
    }
    return offset;
}

void Channel::setChannelConfig(const dto::ChannelConfig &config)
{
    ::memcpy(aeadTransmitKey, config.AeadTransmitKey, aeadModeKeySize);
    ::memcpy(aeadReceiveKey, config.AeadReceiveKey, aeadModeKeySize);
    ChannelTag = config.ChannelTag;
}
