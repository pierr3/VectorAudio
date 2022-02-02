#pragma once
#include "afv_native_export.h"
#include <string>
#include <map>
#include <vector>

namespace afv_native::api {
    class atcClient {

        public:
            AFV_NATIVE_EXPORT atcClient(std::string clientName, std::string resourcePath = "");
            AFV_NATIVE_EXPORT ~atcClient();

            AFV_NATIVE_EXPORT bool IsInitialized();

            AFV_NATIVE_EXPORT void SetCredentials(std::string username, std::string password);

            AFV_NATIVE_EXPORT void SetCallsign(std::string callsign);
            AFV_NATIVE_EXPORT void SetClientPosition(double lat, double lon, double amslm, double aglm);

            AFV_NATIVE_EXPORT bool IsVoiceConnected();
            AFV_NATIVE_EXPORT bool IsAPIConnected();

            AFV_NATIVE_EXPORT bool Connect();
            AFV_NATIVE_EXPORT void Disconnect();

            AFV_NATIVE_EXPORT void SetAudioApi(unsigned int api);
            AFV_NATIVE_EXPORT std::map<unsigned int, std::string> GetAudioApis();

            AFV_NATIVE_EXPORT void SetAudioInputDevice(std::string inputDevice);
            AFV_NATIVE_EXPORT std::vector<std::string> GetAudioInputDevices(unsigned int mAudioApi);
            AFV_NATIVE_EXPORT void SetAudioOutputDevice(std::string outputDevice);
            AFV_NATIVE_EXPORT std::vector<std::string> GetAudioOutputDevices(unsigned int mAudioApi);

            AFV_NATIVE_EXPORT double GetInputPeak() const;
            AFV_NATIVE_EXPORT double GetInputVu() const;

            AFV_NATIVE_EXPORT void SetEnableInputFilters(bool enableInputFilters);
            AFV_NATIVE_EXPORT void SetEnableOutputEffects(bool enableEffects);
            AFV_NATIVE_EXPORT bool GetEnableInputFilters() const;

            AFV_NATIVE_EXPORT void StartAudio();
            AFV_NATIVE_EXPORT void StopAudio();
            AFV_NATIVE_EXPORT bool IsAudioRunning();

            AFV_NATIVE_EXPORT void SetTx(unsigned int freq, bool active);
            AFV_NATIVE_EXPORT void SetRx(unsigned int freq, bool active);
            AFV_NATIVE_EXPORT void SetXc(unsigned int freq, bool active);

            AFV_NATIVE_EXPORT bool GetTxActive(unsigned int freq);
            AFV_NATIVE_EXPORT bool GetRxActive(unsigned int freq);

            AFV_NATIVE_EXPORT bool GetTxState(unsigned int freq);
            AFV_NATIVE_EXPORT bool GetRxState(unsigned int freq);
            AFV_NATIVE_EXPORT bool GetXcState(unsigned int freq);

            // Use this to set the current transceivers to the transceivers from this station, pulled from the AFV database, only one at a time can be active
            AFV_NATIVE_EXPORT void UseTransceiversFromStation(std::string station, int freq);

            AFV_NATIVE_EXPORT void FetchTransceiverInfo(std::string station);

            AFV_NATIVE_EXPORT int GetTransceiverCountForStation(std::string station);

            AFV_NATIVE_EXPORT void SetPtt(bool pttState);

            AFV_NATIVE_EXPORT std::string LastTransmitOnFreq(unsigned int freq);

            AFV_NATIVE_EXPORT void AddFrequency(unsigned int freq);
            AFV_NATIVE_EXPORT void RemoveFrequency(unsigned int freq);
            AFV_NATIVE_EXPORT bool IsFrequencyActive(unsigned int freq);
    };
}