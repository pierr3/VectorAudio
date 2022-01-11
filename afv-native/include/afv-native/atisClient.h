//
//  atisClient.h
//  afv_native
//
//  Created by Mike Evans on 11/18/20.
//

#ifndef atisClient_h
#define atisClient_h

//#include "afv-native/afv/RadioSimulation.h"
//#include "afv-native/afv/ATCRadioStack.h"

#include <memory>
#include <event2/event.h>

#include "afv-native/event.h"
#include "afv-native/afv/APISession.h"
#include "afv-native/afv/EffectResources.h"
#include "afv-native/afv/VoiceSession.h"
#include "afv-native/afv/VoiceCompressionSink.h"
#include "afv-native/afv/dto/Transceiver.h"
#include "afv-native/audio/AudioDevice.h"
#include "afv-native/audio/SpeexPreprocessor.h"
#include "afv-native/audio/SourceToSinkAdapter.h"
#include "afv-native/audio/ITick.h"
#include "afv-native/event/EventCallbackTimer.h"
#include "afv-native/http/EventTransferManager.h"
#include "afv-native/http/RESTRequest.h"
#include "afv-native/audio/WavSampleStorage.h"



namespace afv_native {
    /** ATCClient provides a fully functional ATC Client that can be integrated into
     * an application.
     */
    class ATISClient :
        public audio::ISampleSink,
        public afv::ICompressedFrameSink,
        public std::enable_shared_from_this<ATISClient>,
        public audio::ITick
    {
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
        ATISClient(
            struct event_base* evBase,
            std::string atisFile,
            const std::string& clientName = "AFV-Native",
            std::string baseUrl = "https://voice1.vatsim.uk");

        virtual ~ATISClient();

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

        void setFrequency(unsigned int freq);


        /** sets the (linear) gain to be applied to radioNum */
        void setRadioGain(unsigned int radioNum, float gain);


        /** setCredentials sets the user Credentials for this client.
         *
         * @note This only affects future attempts to connect.
         *
         * @param username The user's CID or username.
         * @param password The user's password
         */
        void setCredentials(const std::string& username, const std::string& password);

        /** setCallsign sets the user's callsign for this client.
         *
         * @note This only affects future attempts to connect.
         *
         * @param callsign the callsign to use.
         */
        void setCallsign(std::string callsign);


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

        void tick();

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
        util::ChainedCallback<void(ClientEventType, void*)>  ClientEventCallback;


        std::map<std::string, std::vector<afv::dto::StationTransceiver>> getStationTransceivers() const;

        void startAudio();
        void stopAudio();

        bool isPlaying();

        void putAudioFrame(const audio::SampleType* bufferIn);


    protected:

        struct event_base* mEvBase;


        http::EventTransferManager mTransferManager;
        afv::APISession mAPISession;
        afv::VoiceSession mVoiceSession;

        void processCompressedFrame(std::vector<unsigned char> compressedData);

        double mClientLatitude;
        double mClientLongitude;
        double mClientAltitudeMSLM;
        double mClientAltitudeGLM;
        unsigned int mFrequency;


        std::string mCallsign;

        void sessionStateCallback(afv::APISessionState state);
        void voiceStateCallback(afv::VoiceSessionState state);

        void sendCachedFrame();


        std::vector<afv::dto::Transceiver> makeTransceiverDto();
        /* sendTransceiverUpdate sends the update now, in process.
         * queueTransceiverUpdate schedules it for the next eventloop.  This is a
         * critical difference as it prevents bad reentrance as the transceiver
         * update callback can trigger a second update if the desired state is
         * out of sync! */
        void sendTransceiverUpdate();
        void queueTransceiverUpdate();
        void stopTransceiverUpdate();



    protected:
        event::EventCallbackTimer mTransceiverUpdateTimer;
        cryptodto::UDPChannel* mChannel;
        std::atomic<uint32_t> mTxSequence;
        std::shared_ptr<afv::VoiceCompressionSink> mVoiceSink;
        std::shared_ptr<audio::SpeexPreprocessor> mVoiceFilter;
        std::shared_ptr<audio::WavSampleStorage> mWavSampleStorage;
        std::shared_ptr<audio::RecordedSampleSource> mRecordedSampleSource;
        std::shared_ptr<audio::SourceToSinkAdapter> mAdapter;
        std::vector<std::vector<unsigned char>> mStoredData;
        bool looped;
        bool playCachedData;
        unsigned int cacheNum;
        std::string mClientName;
        std::string mATISFileName;
    public:
    };
}

#endif /* atisClient_h */
