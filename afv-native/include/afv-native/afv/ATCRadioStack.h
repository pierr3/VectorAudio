//
//  ATCRadioStack.h
//  afv_native
//
//  Created by Mike Evans on 10/21/20.
//

#ifndef ATCRadioStack_h
#define ATCRadioStack_h

#include <memory>
#include <unordered_map>
#include <event2/event.h>

#include "afv-native/event.h"
#include "afv-native/utility.h"
#include "afv-native/afv/EffectResources.h"
#include "afv-native/afv/RemoteVoiceSource.h"
#include "afv-native/afv/RollingAverage.h"
#include "afv-native/afv/VoiceCompressionSink.h"
#include "afv-native/afv/dto/voice_server/AudioRxOnTransceivers.h"
#include "afv-native/afv/dto/Transceiver.h"
#include "afv-native/afv/dto/StationTransceiver.h"
//#include "afv-native/audio/ISampleSink.h"
#include "afv-native/audio/ISampleSource.h"
#include "afv-native/audio/ITick.h"
//#include "afv-native/audio/OutputMixer.h"
#include "afv-native/audio/PinkNoiseGenerator.h"
#include "afv-native/audio/SineToneSource.h"
#include "afv-native/audio/SpeexPreprocessor.h"
#include "afv-native/audio/VHFFilterSource.h"
#include "afv-native/cryptodto/UDPChannel.h"
#include "afv-native/event/EventCallbackTimer.h"
#include "afv-native/util/ChainedCallback.h"


namespace afv_native {
    namespace afv {
    
    class ATCRadioStack;
    
    
    class OutputAudioDevice:
        public audio::ISampleSource
    {
    public:
        
        
        std::weak_ptr<ATCRadioStack> mStack;
        bool isHeadset;
        
        OutputAudioDevice(std::weak_ptr<ATCRadioStack> stack, bool isHeadset);
        
        audio::SourceStatus getAudioFrame(audio::SampleType *bufferOut) override;
        
        
        
    };
    
    
    class OutputDeviceState {
    public:
        /** mChannelBuffer is our single-radio/channel workbuffer - we do our per-channel fx mixing in here before
         * we mix into the mMixingBuffer
        */
        audio::SampleType *mChannelBuffer;

        /** mMixingBuffer is our aggregated mixing buffer for all radios/channels - when we're finished mixing and
         * the final effects pass, we copy this to the output/target buffer.
         */
        audio::SampleType *mMixingBuffer;

        audio::SampleType *mFetchBuffer;
        
        OutputDeviceState();
        virtual ~OutputDeviceState();
        
    };
    
    
    
        /** RadioState is the internal state object for each radio within a RadioSimulation.
         *
         * It tracks the current playback position of the mixing effects, the channel frequency and gain, and the transceivers.
         */
        class ATCRadioState {
        public:
            unsigned int Frequency;
            float Gain=1.0;
            std::shared_ptr<audio::RecordedSampleSource> Click;
            std::shared_ptr<audio::PinkNoiseGenerator> WhiteNoise;
            std::shared_ptr<audio::RecordedSampleSource> Crackle;
            std::shared_ptr<audio::SineToneSource> BlockTone;
            audio::VHFFilterSource vhfFilter;
            std::string lastTransmitCallsign;
            int mLastRxCount;
            bool mBypassEffects;
            bool onHeadset = true; // If we're not on the headset, we're on the Speaker
            bool tx = false;
            bool rx = false;
            std::vector<dto::Transceiver> transceivers;
        };

    
    
        class ATCRadioStack: public std::enable_shared_from_this<ATCRadioStack>,
            public audio::ISampleSink,
            public ICompressedFrameSink
        {
        public:
            ATCRadioStack(struct event_base * evBase,
                          std::shared_ptr<EffectResources> resources,
                          cryptodto::UDPChannel *channel);
            virtual ~ATCRadioStack();
            void setupDevices(util::ChainedCallback<void(ClientEventType,void*)> * eventCallback);
            void rxVoicePacket(const afv::dto::AudioRxOnTransceivers &pkt);
            void setPtt(bool pressed);
            void setRT(bool active);
            void setUDPChannel(cryptodto::UDPChannel *newChannel);
            void setCallsign(const std::string &newCallsign);
            void setClientPosition(double lat, double lon, double amslm, double aglm);
            void addFrequency(unsigned int freq, bool onHeadset);
            
            bool getTxActive(unsigned int freq);
            bool getRxActive(unsigned int freq);
            
            void setRx(unsigned int freq, bool rx);
            void setTx(unsigned int freq, bool tx);
            
            void setTick(std::shared_ptr<audio::ITick> tick);
            
            void setTransceivers(unsigned int freq, std::vector<afv::dto::StationTransceiver> transceivers);
            std::vector<afv::dto::Transceiver> makeTransceiverDto();
            void setOnHeadset(unsigned int freq, bool onHeadset);
            void setGain(unsigned int freq, float gain);
            
            bool getEnableInputFilters() const;
            void setEnableInputFilters(bool enableInputFilters);

            void setEnableOutputEffects(bool enableEffects);
            std::string lastTransmitOnFreq(unsigned int freq);
            std::shared_ptr<audio::ISampleSource> speakerDevice() { return mSpeakerDevice; }
            std::shared_ptr<audio::ISampleSource> headsetDevice() { return mHeadsetDevice; }
            
            audio::SourceStatus getAudioFrame(audio::SampleType *bufferOut, bool onHeadset);
            void putAudioFrame(const audio::SampleType *bufferIn) override;
            
            double getVu() const;
            double getPeak() const;
            void reset();
            
            std::atomic<uint32_t> IncomingAudioStreams;
            
        protected:
            
            static const int maintenanceTimerIntervalMs = 30 * 1000;
            struct event_base *mEvBase;
            cryptodto::UDPChannel *mChannel;
            std::string mCallsign;
            
            util::ChainedCallback<void(ClientEventType,void*)>  * ClientEventCallback;
            
            std::mutex mStreamMapLock;
            std::unordered_map<std::string, struct CallsignMeta> mIncomingStreams;
            
            std::mutex mRadioStateLock;
            std::map<unsigned int, ATCRadioState> mRadioState;
            
            std::atomic<bool> mPtt;
            std::atomic<bool> mRT;
            bool mLastFramePtt;
            
            std::shared_ptr<audio::SpeexPreprocessor> mVoiceFilter;
            std::shared_ptr<OutputAudioDevice> mHeadsetDevice;
            std::shared_ptr<OutputAudioDevice> mSpeakerDevice;
            
            std::shared_ptr<audio::ITick> mTick;
            
            
            
            std::atomic<uint32_t> mTxSequence;
            std::shared_ptr<VoiceCompressionSink> mVoiceSink;
            std::shared_ptr<OutputDeviceState> mHeadsetState;
            std::shared_ptr<OutputDeviceState> mSpeakerState;
            std::shared_ptr<EffectResources> mResources;
            
            
            event::EventCallbackTimer mMaintenanceTimer;
            RollingAverage<double> mVuMeter;
            void processCompressedFrame(std::vector<unsigned char> compressedData) override;
            void maintainIncomingStreams();
            
            double mClientLatitude;
            double mClientLongitude;
            double mClientAltitudeMSLM;
            double mClientAltitudeGLM;

            
            void resetRadioFx(unsigned int radio, bool except_click = false);
            bool mix_effect(std::shared_ptr<audio::ISampleSource> effect, float gain, std::shared_ptr<OutputDeviceState> state);
            void set_radio_effects(size_t rxIter, float crackleGain, float &whiteNoiseGain);
            
        private:
            bool _process_radio(
                    const std::map<void *, audio::SampleType[audio::frameSizeSamples]> &sampleCache,
                    std::map<void *, audio::SampleType[audio::frameSizeSamples]> &eqSampleCache,
                    size_t rxIter, std::shared_ptr<OutputDeviceState> state);
            
            /** mix_buffers is a utility function that mixes two buffers of audio together.  The src_dst
             * buffer is assumed to be the final output buffer and is modified by the mixing in place.
             * src2 is read-only and will be scaled by the provided linear gain.
             *
             * @note as this mixer has some really fundamental assumptions about buffer alignment which
             * are required as it uses SSE2 intrinsics to perform the mixing.  These requirements are
             * met inside elsewhere inside this class.
             *
             * @param src_dst pointer to the source and destination buffer.
             * @param src2 pointer to the origin of the samples to mix in.
             * @param src2_gain linear gain to apply to src2.
             */
            static void mix_buffers(audio::SampleType * RESTRICT src_dst, const audio::SampleType * RESTRICT src2, float src2_gain = 1.0);
            bool _packetListening(const afv::dto::AudioRxOnTransceivers &pkt);
            
    
        };

    
    
    }
}



#endif /* ATCRadioStack_h */
