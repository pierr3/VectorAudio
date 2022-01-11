/* util/base64.cpp
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

#include "afv-native/util/base64.h"

#include <cstdint>
#include <string>
#include <openssl/evp.h>
#include <vector>

using namespace std;
using namespace afv_native::util;

string
afv_native::util::Base64Encode(const unsigned char *buffer_in, size_t len)
{
    /* NOTE(CC):
     *
     * This implementation bounces through a temporary buffer resulting in a
     * double-copy.
     *
     * This is chosen in preference to a no-copy approach to prevent
     * buffer-overflows as we cannot limit the output space required by
     * EVP_EncodeBlock.
     *
     * Similarly, we don't use the one-copy approach as it always wants to write
     * the terminal character, whereas that's not (portable) compatible with the
     * C++ string class as we're generally not permitted to mess with the
     * terminating character! >_<
    */

    // output size required by EVP_EncodeBlock is predictable - see EVP_EncodeBlock(3)
    const size_t output_len = (((len + 2) / 3) * 4) + 1;

    std::vector<unsigned char> out_buf(output_len);
    EVP_EncodeBlock(out_buf.data(), buffer_in, len);

    string base64_value(reinterpret_cast<char *>(out_buf.data()));
    return std::move(base64_value);
}


size_t
afv_native::util::Base64DecodeLen(size_t input_len)
{
    return ((input_len + 3) / 4) * 3;
}

size_t
afv_native::util::Base64Decode(const string &base64_in, unsigned char *buffer_out, size_t len)
{
    // because the OpenSSL methods are completely unguarded, we need to preempt
    // a few things - we need to limit the data exposed to the decode function
    // to the maximum length we can store in the output buffer. >_<

    size_t input_len = base64_in.length();
    if (Base64DecodeLen(input_len) > len) {
        input_len = (len / 3) * 4;
    }
    return EVP_DecodeBlock(buffer_out, reinterpret_cast<const unsigned char *>(base64_in.c_str()), input_len);
}
