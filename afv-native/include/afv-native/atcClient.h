//
//  atcClient.hpp
//  afv_native
//
//  Created by Mike Evans on 10/18/20.
//

#ifndef atcClient_h
#define atcClient_h

#include "afv-native/afv/RadioSimulation.h"
#include "afv-native/afv/ATCRadioStack.h"

#include <memory>
#include <event2/event.h>

#include "afv-native/event.h"
#include "afv-native/afv/APISession.h"
#include "afv-native/afv/EffectResources.h"
#include "afv-native/afv/VoiceSession.h"
#include "afv-native/afv/dto/Transceiver.h"
#include "afv-native/audio/AudioDevice.h"
#include "afv-native/event/EventCallbackTimer.h"
#include "afv-native/http/EventTransferManager.h"
#include "afv-native/http/RESTRequest.h"

namespace afv_native {
    /** ATCClient provides a fully functional ATC Client that can be integrated into
     * an application.
     */
    class ATCClient {
    public:
        /** Construct an AFV-native ATC Client.
         *
         * The pilot client will be in the disconnected state and ready to have
         * the credentials, position and other configuration options set before
         * attempting to connect.
         *
         * The containing client must provide and run a libevent eventloop for
         * AFV-native to attach its operations against, and must ensure that
         * this loop is run constantly, even when the client is not connected.
         * (It's used for some tear-down operations which must run to completion
         * after the client is shut-down if possible.)
         *
         * @param evBase an initialised libevent event_base to register the client's
         *      asynchronous IO and deferred operations against.
         * @param resourceBasePath A relative or absolute path to where the AFV-native
         *      resource files are located.
         * @param baseUrl The baseurl for the AFV API server to connect to.  The
         *      default should be used in most cases.
         * @param clientName The name of this client to advertise to the
         *      audio-subsystem.
         */
        ATCClient(
                struct event_base *evBase,
                const std::string &resourceBasePath,
                const std::string &clientName = "AFV-Native",
                std::string baseUrl = "https://voice1.vatsim.uk");

        virtual ~ATCClient();

        /** setBaseUrl is used to change the API URL.
         *
         * @note This affects all future API requests, but any in flight will not
         * be cancelled and resent.
         *
         * @param newUrl the new URL (without at trailing slash).
         */
        void setBaseUrl(std::string newUrl);

        /** set the ClientPosition to report with the next transceiver update.
         *
         * @param lat the client latitude in decimal degrees.
         * @param lon the client longitude in decimal degrees.
         * @param amslm the client's altitude above mean-sea-level in meters.
         * @param aglm the client's altitude above ground level in meters.
         */
        void setClientPosition(double lat, double lon, double amslm, double aglm);

        /** set the radio the Ptt will control.
         *
         * @param freq the frequency to set the transmit state
         * @param active If the frequency is transmitting or not
         *
         */
        void setTx(unsigned int freq, bool active);
        
        
        void setRx(unsigned int freq, bool active);

        /** sets the (linear) gain to be applied to radioNum */
        void setRadioGain(unsigned int radioNum, float gain);

        /** sets the PTT (push-to-talk) state for the radio.
         *
         * @note If the radio frequencies are out of sync with the server, this will
         * initiate an immediate transceiver set update and the Ptt will remain
         * blocked/guarded until the update has been acknowledged.
         *
         * @param pttState true to start transmitting, false otherwise.
         */
        void setPtt(bool pttState);
        
        
        void setRT(bool rtState);
        
        void setOnHeadset(unsigned int freq, bool onHeadset);

        /** setCredentials sets the user Credentials for this client.
         *
         * @note This only affects future attempts to connect.
         *
         * @param username The user's CID or username.
         * @param password The user's password
         */
        void setCredentials(const std::string &username, const std::string &password);

        /** setCallsign sets the user's callsign for this client.
         *
         * @note This only affects future attempts to connect.
         *
         * @param callsign the callsign to use.
         */
        void setCallsign(std::string callsign);

        /** set the audioApi (per the audio::AudioDevice definitions) to use when next starting the
         * audio system.
         * @param api an API id
         */
        void setAudioApi(audio::AudioDevice::Api api);

        void setAudioInputDevice(std::string inputDevice);
        void setAudioOutputDevice(std::string outputDevice);
        void setSpeakerOutputDevice(std::string outputDevice);

        /** isAPIConnected() indicates if the API Server connection is up.
         *
         * @return true if the API server connection is good or in the middle of
         * reauthenticating a live session.  False if it is yet to authenticate,
         * or is not connected at all.
         */
        bool isAPIConnected() const;
        bool isVoiceConnected() const;

        /** connect() starts the Client connecting to the API server, then establishing the voice session.
         *
         * In order for this to work, the credentials, callsign and audio device must be configured first,
         * and you should have already set the clientPosition and radio states once.
         *
         * @return true if the connection process was able to start, false if key data is missing or an
         *  early failure occured.
         */
        bool connect();

        /** disconnect() tears down the voice session and discards the authentication session data.
         *
         */
        void disconnect();

        double getInputPeak() const;
        double getInputVu() const;

        bool getEnableInputFilters() const;
        void setEnableInputFilters(bool enableInputFilters);
        void setEnableOutputEffects(bool enableEffects);

        /** ClientEventCallback provides notifications when certain client events occur.  These can be used to
         * provide feedback within the client itself without needing to poll Client's methods.
         *
         * The callbacks take two paremeters-  the first is the ClientEventType which informs the client what type
         * of event occured.
         *
         * The second argument is a pointer to data relevant to the callback.  The memory it points to is only
         * guaranteed to be available for the duration of the callback.
         */
        util::ChainedCallback<void(ClientEventType,void*)>  ClientEventCallback;

        /** getStationAliases returns a vector of all the known station aliases.
         *
         * @note this method uses a copy in place to prevent race inside the
         *  client code and consumers and is consequentially expensive.  Please
         *  only call it after you get a notification of there being a change,
         *  and even then, only once.
         */
        std::vector<afv::dto::Station> getStationAliases() const;
        std::map<std::string, std::vector<afv::dto::StationTransceiver>> getStationTransceivers() const;
        std::string lastTransmitOnFreq(unsigned int freq);
        void startAudio();
        void stopAudio();

        /** logAudioStatistics dumps the internal data about over/underflow totals to the AFV log.
         *
         */
        void logAudioStatistics();

        std::shared_ptr<const audio::AudioDevice> getAudioDevice() const;

        /** getRxActive returns if the nominated radio is currently Receiving voice, irrespective as to if it's audiable
         * or not.
         *
         * @param radioNumber the number (starting from 0) of the radio to probe
         * @return true if the radio would have voice to play, false otherwise.
         */
        bool getRxActive(unsigned int radioNumber);

        /** getTxActive returns if the nominated radio is currently Transmitting voice.
         *
         * @param radioNumber the number (starting from 0) of the radio to probe
         * @return true if the radio is transmitting from the mic, false otherwise.
         */
        bool getTxActive(unsigned int radioNumber);
        
        /** requestStationTransceivers requests the list of transceivers associated with the named station
         *
         *  @param inStation the name of the station to list the transceivers
         */
        void requestStationTransceivers(std::string inStation);
        
        void addFrequency(unsigned int freq, bool onHeadset);
        void linkTransceivers(std::string callsign, unsigned int freq);
        
        void setTick(std::shared_ptr<audio::ITick> tick);
        

    protected:

        struct event_base *mEvBase;
        std::shared_ptr<afv::EffectResources> mFxRes;

        http::EventTransferManager mTransferManager;
        afv::APISession mAPISession;
        afv::VoiceSession mVoiceSession;
        std::shared_ptr<afv::ATCRadioStack> mATCRadioStack;
        std::shared_ptr<audio::AudioDevice> mAudioDevice;
        std::shared_ptr<audio::AudioDevice> mSpeakerDevice;
        

        std::string mCallsign;

        void sessionStateCallback(afv::APISessionState state);
        void voiceStateCallback(afv::VoiceSessionState state);

        bool mTxUpdatePending;
        bool mWantPtt;
        bool mPtt;

        
        std::vector<afv::dto::Transceiver> makeTransceiverDto();
        /* sendTransceiverUpdate sends the update now, in process.
         * queueTransceiverUpdate schedules it for the next eventloop.  This is a
         * critical difference as it prevents bad reentrance as the transceiver
         * update callback can trigger a second update if the desired state is
         * out of sync! */
        void sendTransceiverUpdate();
        void queueTransceiverUpdate();
        void stopTransceiverUpdate();

        void aliasUpdateCallback();
        void stationTransceiversUpdateCallback();
        
    private:
        void unguardPtt();
    protected:
        event::EventCallbackTimer mTransceiverUpdateTimer;
        

        std::string mClientName;
        audio::AudioDevice::Api mAudioApi;
        std::string mAudioInputDeviceName;
        std::string mAudioOutputDeviceName;
        std::string mAudioSpeakerDeviceName;
        
    public:
    };
}

#endif /* atcClient_h */
