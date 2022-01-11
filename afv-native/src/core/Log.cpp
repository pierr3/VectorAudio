/* core/Log.cpp
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

#include "afv-native/Log.h"

#include <cstdarg>
#include <ctime>
#include <vector>
#include <sstream>
#include <ios>
#include <iomanip>
#include <mutex>

using namespace std;
static FILE *gLoggerFh = nullptr;

static void
cleanUpDefaultLogger()
{
    if (nullptr != gLoggerFh) {
        fclose(gLoggerFh);
        gLoggerFh = nullptr;
    }
}

static void
defaultLogger(const char *subsystem, const char *file, int line, const char *outputLine)
{
    char dateTimeBuf[100];
    time_t t = time(nullptr);
    strftime(dateTimeBuf, 100, "%c", localtime(&t));
    if (nullptr == gLoggerFh) {
        gLoggerFh = fopen("afv.log", "at");
        atexit(cleanUpDefaultLogger);
    }
    if (nullptr != gLoggerFh) {
#ifdef NDEBUG
        fprintf(gLoggerFh, "%s: %s: %s\n", dateTimeBuf, subsystem, outputLine);
#else
        fprintf(gLoggerFh, "%s: %s: %s(%d): %s\n", dateTimeBuf, subsystem, file, line, outputLine);
#endif
        fflush(gLoggerFh);
    }
}

static afv_native::log_fn  gLogger = defaultLogger;
static std::mutex gLoggerLock;

void afv_native::__Log(const char *file, int line, const char *subsystem, const char *format, ...)
{
    if (gLogger == nullptr) {
        return;
    }
    va_list ap;
    va_list ap2;
            va_start(ap, format);
    va_copy(ap2, ap);
    size_t outputLen = vsnprintf(nullptr, 0, format, ap2)+1;

    std::vector<char> outBuffer(outputLen);
    vsnprintf(outBuffer.data(), outputLen, format, ap);
    {
        std::lock_guard<std::mutex> logLock(gLoggerLock);

        gLogger(subsystem, file, line, outBuffer.data());
    }
    va_end(ap2);
    va_end(ap);
}

void afv_native::setLogger(afv_native::log_fn newLogger) {
    gLogger = newLogger;
}

void afv_native::__Dumphex(const char *file, int line, const char *subsystem, const void *buf, size_t len)
{
    for (size_t i = 0; i < len;) {
        std::stringstream lineout;

        // splat the address.
        lineout << std::right << std::hex << std::setw(4) << std::setfill('0') << i;

        lineout << ": ";
        for (int lineCount = 0;lineCount < 16 && i < len; lineCount++) {
            lineout << " ";
            lineout.width(2);
            lineout.fill('0');
            lineout << std::right << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(reinterpret_cast<const uint8_t *>(buf)[i++]);
        }
        {
            std::lock_guard<std::mutex> logLock(gLoggerLock);

            gLogger(subsystem, file, line, lineout.str().c_str());
        }
    }
}
