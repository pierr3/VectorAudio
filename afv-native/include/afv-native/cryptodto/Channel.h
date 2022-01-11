/* cryptodto/Channel.h
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

#ifndef AFV_NATIVE_CHANNEL_H
#define AFV_NATIVE_CHANNEL_H

#include <vector>
#include <string>
#include <cstdint>
#include <openssl/evp.h>
#include <msgpack.hpp>

#include "afv-native/cryptodto/params.h"
#include "afv-native/cryptodto/SequenceTest.h"
#include "afv-native/cryptodto/dto/ICryptoDTO.h"
#include "afv-native/Log.h"

namespace afv_native {
    namespace cryptodto {
        namespace dto {
            class ChannelConfig;

            class Header;
        }

        class Channel {
        protected:
            unsigned char aeadTransmitKey[aeadModeKeySize];
            unsigned char aeadReceiveKey[aeadModeKeySize];

            static void make_aead_key(unsigned char keyBuffer[]);

            size_t decryptChaCha20Poly1305(
                    unsigned char *bodyOut,
                    const unsigned char *cipherIn,
                    size_t cipherLen,
                    const dto::Header &header,
                    const unsigned char *aadIn,
                    size_t aadLen);

            size_t encryptChaCha20Poly1305(
                    unsigned char *cipherOut,
                    const unsigned char *plainIn,
                    size_t plainLen,
                    const dto::Header &header,
                    const unsigned char *aadIn,
                    size_t aadLen);

            static void makeChaCha20Poly1305Nonce(uint64_t sequence, unsigned char *nonceBuffer);

        protected:

            /** encodeDto takes the DTO provided as dto and encodes it into dtoBuf.
             *
             * This forms the ciphertext portion of an encrypted DTO message.
             *
             * @tparam T type of the DTO.  T must provide a getName() method that
             *          returns the DTO name, and be encodable by msgpack-c.
             * @param dtoBuf the sbuffer object to pass the encoded dto out in.
             * @param dto the dto to encode
             * @return true if the message was successfully encoded, false otherwise.
             */
            template<class T>
            static bool encodeDto(msgpack::sbuffer &dtoBuf, const T &dto)
            {
                // assemble the body and pack it.
                std::string dtoName = dto.getName();
                uint16_t nLen = static_cast<uint16_t>(dtoName.length());
                dtoBuf.write(reinterpret_cast<char *>(&nLen), 2);
                dtoBuf.write(dtoName.data(), nLen);

                // because we don't know the dto size in advance, pack it into it's own
                // temp buffer, then copy it into the actual payload buffer.
                msgpack::sbuffer dtoTempBuf;
                msgpack::pack(dtoTempBuf, dto);
                nLen = static_cast<uint16_t>(dtoTempBuf.size());
                if (dtoTempBuf.size() > UINT16_MAX) {
                    return false;
                }

                dtoBuf.write(reinterpret_cast<char *>(&nLen), 2);
                dtoBuf.write(dtoTempBuf.data(), dtoTempBuf.size());

                assert(dtoBuf.size() == (dtoTempBuf.size() + 2 + dtoName.size() + 2));
                return true;
            }

        public:
            std::string ChannelTag;
            time_t LastTransmit;
            time_t LastReceive;

            explicit Channel();

            virtual void setChannelConfig(const dto::ChannelConfig &config);

            template<class T>
            size_t Encapsulate(
                    unsigned char *bufOut,
                    size_t bufOutLen,
                    sequence_t sequence,
                    cryptodto::CryptoDtoMode mode,
                    const T &dto)
            {
                msgpack::sbuffer dtoBuf;
                if (!encodeDto(dtoBuf, dto)) {
                    return 0;
                }
                // use the generic encapsulate method.
                return Encapsulate(
                        reinterpret_cast<const unsigned char *>(dtoBuf.data()),
                        dtoBuf.size(),
                        sequence,
                        mode,
                        bufOut,
                        bufOutLen);
            }

            size_t Encapsulate(
                    const unsigned char *plainTextBuf,
                    size_t plainTextLen,
                    sequence_t sequence,
                    cryptodto::CryptoDtoMode mode,
                    unsigned char *cipherTextBufOut,
                    size_t cipherTextLen);

            bool Decapsulate(
                    const unsigned char *cipherTextIn,
                    size_t cipherTextLen,
                    std::string &channelTag,
                    sequence_t &sequence,
                    CryptoDtoMode &modeOut,
                    std::string &dtoNameOut,
                    msgpack::sbuffer &dtoOut);
        };
    }
}

#endif //AFV_NATIVE_CHANNEL_H
