#pragma once
#include "afv_native_export.h"
#include "event.h"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include "hardwareType.h"
#include "afv-native/Log.h"

namespace afv_native::api {
    class atcClient {

        public:
            AFV_NATIVE_API static void setLogger(afv_native::log_fn gLogger);

            AFV_NATIVE_API explicit atcClient(std::string clientName, std::string resourcePath = "");
            AFV_NATIVE_API ~atcClient();

            AFV_NATIVE_API bool IsInitialized();

            AFV_NATIVE_API void SetCredentials(std::string username, std::string password);

            AFV_NATIVE_API void SetCallsign(std::string callsign);
            AFV_NATIVE_API void SetClientPosition(double lat, double lon, double amslm, double aglm);

            AFV_NATIVE_API bool IsVoiceConnected();
            AFV_NATIVE_API bool IsAPIConnected();

            AFV_NATIVE_API bool Connect();
            AFV_NATIVE_API void Disconnect();

            AFV_NATIVE_API void SetAudioApi(unsigned int api);
            AFV_NATIVE_API std::map<unsigned int, std::string> GetAudioApis();

            AFV_NATIVE_API void SetAudioInputDevice(std::string inputDevice);
            AFV_NATIVE_API std::vector<std::string> GetAudioInputDevices(unsigned int mAudioApi);
            AFV_NATIVE_API void SetAudioOutputDevice(std::string outputDevice);
            AFV_NATIVE_API void SetAudioSpeakersOutputDevice(std::string outputDevice);
            AFV_NATIVE_API void SetHeadsetOutputChannel(int channel);
            AFV_NATIVE_API std::vector<std::string> GetAudioOutputDevices(unsigned int mAudioApi);

            AFV_NATIVE_API double GetInputPeak() const;
            AFV_NATIVE_API double GetInputVu() const;

            AFV_NATIVE_API void SetEnableInputFilters(bool enableInputFilters);
            AFV_NATIVE_API void SetEnableOutputEffects(bool enableEffects);
            AFV_NATIVE_API bool GetEnableInputFilters() const;

            AFV_NATIVE_API void StartAudio();
            AFV_NATIVE_API void StopAudio();
            AFV_NATIVE_API bool IsAudioRunning();

            AFV_NATIVE_API void SetTx(unsigned int freq, bool active);
            AFV_NATIVE_API void SetRx(unsigned int freq, bool active);
            AFV_NATIVE_API void SetXc(unsigned int freq, bool active);
            AFV_NATIVE_API void SetOnHeadset(unsigned int freq, bool active);

            AFV_NATIVE_API bool GetTxActive(unsigned int freq);
            AFV_NATIVE_API bool GetRxActive(unsigned int freq);
            AFV_NATIVE_API bool GetOnHeadset(unsigned int freq);

            AFV_NATIVE_API bool GetTxState(unsigned int freq);
            AFV_NATIVE_API bool GetRxState(unsigned int freq);
            AFV_NATIVE_API bool GetXcState(unsigned int freq);

            // Use this to set the current transceivers to the transceivers from this station, pulled from the AFV database, only one at a time can be active
            AFV_NATIVE_API void UseTransceiversFromStation(std::string station, int freq);

            AFV_NATIVE_API void FetchTransceiverInfo(std::string station);
            AFV_NATIVE_API void FetchStationVccs(std::string station);
            AFV_NATIVE_API void GetStation(std::string station);

            AFV_NATIVE_API int GetTransceiverCountForStation(std::string station);

            AFV_NATIVE_API void SetPtt(bool pttState);

            AFV_NATIVE_API void SetAtisRecording(bool state);
            AFV_NATIVE_API bool IsAtisRecording();

            AFV_NATIVE_API void SetAtisListening(bool state);
            AFV_NATIVE_API bool IsAtisListening();

            AFV_NATIVE_API void StartAtisPlayback(std::string callsign, unsigned int freq);
            AFV_NATIVE_API void StopAtisPlayback();
            AFV_NATIVE_API bool IsAtisPlayingBack();

            AFV_NATIVE_API std::string LastTransmitOnFreq(unsigned int freq);

            AFV_NATIVE_API void SetRadiosGain(float gain);

            AFV_NATIVE_API void AddFrequency(unsigned int freq, std::string stationName = "");
            AFV_NATIVE_API void RemoveFrequency(unsigned int freq);
            AFV_NATIVE_API bool IsFrequencyActive(unsigned int freq);

            AFV_NATIVE_API void SetHardware(afv_native::HardwareType hardware);

            AFV_NATIVE_API void RaiseClientEvent(std::function<void(afv_native::ClientEventType, void*, void*)> callback);
    };
}