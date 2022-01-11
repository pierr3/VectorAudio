#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>

#include "afv-native/Client.h"
#include <event2/event.h>

#ifdef WIN32
#define AFV_API __declspec(dllexport)
#else
#define AFV_API
#endif

struct event_base* ev_base;

std::mutex afvMutex;
std::unique_ptr<afv_native::Client> client;
std::unique_ptr<std::thread> eventThread;
std::atomic<bool>keepAlive{ false };
std::atomic<bool> isInitialized{ false };
afv_native::audio::AudioDevice::Api mAudioApi;
int mInputDevice;
int mOutputDevice;

typedef std::map<afv_native::audio::AudioDevice::Api, std::string> ApiProviderMap;
typedef std::map<int, afv_native::audio::AudioDevice::DeviceInfo> AudioDeviceMap;

std::map<afv_native::audio::AudioDevice::Api, std::string> mAudioProviders;
std::map<int, afv_native::audio::AudioDevice::DeviceInfo> mInputDevices;
std::map<int, afv_native::audio::AudioDevice::DeviceInfo> mOutputDevices;

#ifdef __cplusplus
extern "C" {

    AFV_API void Initialize(char* resourcePath, int numRadios, char* clientName)
    {
        #ifdef WIN32
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD(2, 2);
        WSAStartup(wVersionRequested, &wsaData);
        #endif

        //afv_native::setLogger(nullptr);

        ev_base = event_base_new();
        client = std::make_unique<afv_native::Client>(ev_base, numRadios, resourcePath, clientName);
        mAudioProviders = afv_native::audio::AudioDevice::getAPIs();

        keepAlive = true;
        eventThread = std::make_unique<std::thread>([]
        {
            while (keepAlive)
            {
                event_base_loop(ev_base, EVLOOP_NONBLOCK);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        client->setEnableInputFilters(true);

        isInitialized = true;
    }

    AFV_API void Destroy()
    {
        keepAlive = false;
        if (eventThread->joinable())
        {
            eventThread->join();
        }
        client.reset();
        isInitialized = false;
        #ifdef WIN32
        WSACleanup();
        #endif
    }

    AFV_API bool IsClientInitialized()
    {
        return isInitialized;
    }

    AFV_API void SetBaseUrl(const char* newUrl)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setBaseUrl(std::string(newUrl));
    }

    AFV_API void SetClientPosition(double lat, double lon, double amslm, double aglm)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setClientPosition(lat, lon, amslm, aglm);
    }

    AFV_API void SetRadioState(unsigned int radioNum, int freq)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setRadioState(radioNum, freq);
    }

    AFV_API void SetTxRadio(unsigned int radioNum)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setTxRadio(radioNum);
    }

    AFV_API void SetRadioGain(unsigned int radioNum, float gain)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setRadioGain(radioNum, gain);
    }

    AFV_API void SetPtt(bool pttState)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setPtt(pttState);
    }

    AFV_API void SetCredentials(const char* username, const char* password)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setCredentials(std::string(username), std::string(password));
    }

    AFV_API void SetCallsign(const char* callsign)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setCallsign(std::string(callsign));
    }

    AFV_API bool IsVoiceConnected()
    {
        return client->isVoiceConnected();
    }

    AFV_API bool IsAPIConnected()
    {
        return client->isAPIConnected();
    }

    AFV_API bool AfvConnect()
    {
        return client->connect();
    }

    AFV_API void AfvDisconnect()
    {
        client->disconnect();
    }

    AFV_API double GetInputPeak()
    {
        return client->getInputPeak();
    }

    AFV_API double GetInputVu()
    {
        return client->getInputVu();
    }

    AFV_API bool GetEnableInputFilters()
    {
        return client->getEnableInputFilters();
    }

    AFV_API void SetEnableInputFilters(bool enableInputFilters)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setEnableInputFilters(enableInputFilters);
    }

    AFV_API void SetEnableOutputEffects(bool enableEffects)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setEnableOutputEffects(enableEffects);
    }

    AFV_API void SetEnableHfSquelch(bool enableSquelch)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->setEnableHfSquelch(enableSquelch);
    }

    AFV_API void StartAudio()
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->startAudio();
    }

    AFV_API void StopAudio()
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        client->stopAudio();
    }

    AFV_API void GetAudioApis(void(*callback)(unsigned int api, const char* name))
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        for (const auto& item : mAudioProviders)
        {
            callback(item.first, item.second.c_str());
        }
    }

    AFV_API void SetAudioApi(const char* api)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        for (ApiProviderMap::const_iterator it = mAudioProviders.begin(); it != mAudioProviders.end(); ++it)
        {
            if (it->second == api)
            {
                mAudioApi = it->first;
                mInputDevices = afv_native::audio::AudioDevice::getCompatibleInputDevicesForApi(mAudioApi);
                mOutputDevices = afv_native::audio::AudioDevice::getCompatibleOutputDevicesForApi(mAudioApi);
            }
        }
    }

    AFV_API void GetInputDevices(void(*callback)(unsigned int id, const char* name))
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        for (const auto& item : mInputDevices)
        {
            callback(item.first, item.second.name.c_str());
        }
    }

    AFV_API void GetOutputDevices(void(*callback)(unsigned int id, const char* name))
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        for (const auto& item : mOutputDevices)
        {
            callback(item.first, item.second.name.c_str());
        }
    }

    AFV_API void SetAudioInputDevice(const char* inputDevice)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        for (AudioDeviceMap::const_iterator it = mInputDevices.begin(); it != mInputDevices.end(); ++it)
        {
            if (strcmp(it->second.name.c_str(), inputDevice) == 0)
            {
                mInputDevice = it->first;
                client->setAudioInputDevice(it->second.name.c_str());
            }
        }
    }

    AFV_API void SetAudioOutputDevice(const char* outputDevice)
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        for (AudioDeviceMap::const_iterator it = mOutputDevices.begin(); it != mOutputDevices.end(); ++it)
        {
            if (strcmp(it->second.name.c_str(), outputDevice) == 0)
            {
                mOutputDevice = it->first;
                client->setAudioOutputDevice(it->second.name.c_str());
            }
        }
    }

    AFV_API void GetStationAliases(void(*callback)(const char* id, const char* callsign,
        unsigned int frequency, unsigned int frequencyAlias))
    {
        std::lock_guard<std::mutex> lock(afvMutex);
        for (const auto& station : client->getStationAliases())
        {
            callback(station.ID.c_str(), station.Name.c_str(), station.Frequency, station.FrequencyAlias);
        }
    }

    AFV_API void RaiseClientEvent(void(*callback)(afv_native::ClientEventType evt, int data))
    {
        client->ClientEventCallback.addCallback(nullptr, [callback](afv_native::ClientEventType evt, void* data)
        {
            int v = -1;
            if (data != nullptr)
            {
                v = *reinterpret_cast<int*>(data);
            }
            callback(evt, v);
        });
    }
}
#endif