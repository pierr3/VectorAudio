/* testclient/TestClient.h
 *
 * This file is part of AFV-Native.
 *
 * Copyright (c) 2019, Christopher Collins
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

#ifndef TESTCLIENT_RENDERMANAGER_H
#define TESTCLIENT_RENDERMANAGER_H

#include <string>
#include <memory>
#include <SDL.h>
#include <imgui.h>
#include <event2/event.h>

#include "afv-native/Client.h"

class TestClient
{
    SDL_Window *mWindow;
    SDL_GLContext mContext;
    struct event_base *mEventBase;
    bool mShouldQuit;

    bool mShowDemo;

    ImFontAtlas mFontAtlas;

    std::string mAFVAPIServer = "http://localhost:61000";
    std::string mAFVUsername = "";
    std::string mAFVPassword = "";
    std::string mAFVCallsign = "";


    double mClientLatitude = 34.9;
    double mClientLongitude = -118.500;
    double mClientAltitudeMSLM = 500.0;
    double mClientAltitudeAGLM = 500.0;

    int mTxRadio = 0;
    int mCom1Freq = 125800000;
    float mCom1Gain = 1.0f;
    int mCom2Freq = 124500000;
    float mCom2Gain = 1.0f;
    bool mPTT = false;

    bool mInputFilter = false;
    bool mOutputEffects = false;
    float mPeak = 0.0f;
    float mVu = 0.0f;

    std::shared_ptr<afv_native::Client> mClient;

    std::map<afv_native::audio::AudioDevice::Api,std::string> mAudioProviders;
    std::map<int,afv_native::audio::AudioDevice::DeviceInfo> mInputDevices;
    std::map<int,afv_native::audio::AudioDevice::DeviceInfo> mOutputDevices;
    afv_native::audio::AudioDevice::Api mAudioApi;
    int mInputDevice;
    int mOutputDevice;

    std::string nameForAudioApi(afv_native::audio::AudioDevice::Api apiNum);
    void setAudioApi(afv_native::audio::AudioDevice::Api apiNum);

public:
    TestClient(SDL_Window *win, SDL_GLContext glContext, struct event_base *eventBase);

    bool shouldQuit() const;

    void handleInput();

    void drawFrame();

    void guiMainMenu();

    void setAudioDevice() const;
};


#endif //TESTCLIENT_RENDERMANAGER_H
