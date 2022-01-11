//
//  ATCRadioStack.cpp
//  afv_native
//
//  Created by Mike Evans on 10/21/20.
//

#include <cmath>
#include <atomic>

#include "afv-native/Log.h"
#include "afv-native/afv/ATCRadioStack.h"
#include "afv-native/afv/RadioSimulation.h"
#include "afv-native/afv/dto/voice_server/AudioTxOnTransceivers.h"
#include "afv-native/audio/VHFFilterSource.h"
#include "afv-native/audio/PinkNoiseGenerator.h"


using namespace afv_native;
using namespace afv_native::afv;

const float fxClickGain = 1.0f;

const float fxBlockToneGain = 0.22f;

const float fxWhiteNoiseGain = 0.01f;

const float fxBlockToneFreq = 180.0f;

OutputAudioDevice::OutputAudioDevice(std::weak_ptr<ATCRadioStack> stack, bool isHeadset):
    mStack(stack),
    isHeadset(isHeadset)
{
    
}

audio::SourceStatus OutputAudioDevice::getAudioFrame(audio::SampleType *bufferOut)
{
    return mStack.lock()->getAudioFrame(bufferOut, isHeadset);
}


OutputDeviceState::OutputDeviceState()
{
    mChannelBuffer = new audio::SampleType[audio::frameSizeSamples];
    mMixingBuffer = new audio::SampleType[audio::frameSizeSamples];
    mFetchBuffer = new audio::SampleType[audio::frameSizeSamples];
}

OutputDeviceState::~OutputDeviceState()
{
        delete[] mFetchBuffer;
        delete[] mMixingBuffer;
        delete[] mChannelBuffer;
}


ATCRadioStack::ATCRadioStack(struct event_base * evBase,
                             std::shared_ptr<EffectResources> resources,
                             cryptodto::UDPChannel *channel):
    mEvBase(evBase),
    mChannel(),
    mPtt(false),
    mRT(false),
    IncomingAudioStreams(0),
    mResources(std::move(resources)),
    mStreamMapLock(),
    mIncomingStreams(),
    mLastFramePtt(false),
    mRadioStateLock(),
    mTxSequence(0),
    mRadioState(),
    mVoiceSink(std::make_shared<VoiceCompressionSink>(*this)),
    mMaintenanceTimer(mEvBase, std::bind(&ATCRadioStack::maintainIncomingStreams, this)),
    mVuMeter(300 / audio::frameLengthMs)

{
    setUDPChannel(channel);
    mMaintenanceTimer.enable(maintenanceTimerIntervalMs);
    
    

}

ATCRadioStack::~ATCRadioStack()
{

//    delete[] AudiableAudioStreams;
}

void ATCRadioStack::setupDevices(util::ChainedCallback<void(ClientEventType,void*)> * eventCallback)
{
    mHeadsetDevice = std::make_shared<OutputAudioDevice> (shared_from_this(),true);
    mSpeakerDevice = std::make_shared<OutputAudioDevice> (shared_from_this(),false);
    mHeadsetState = std::make_shared<OutputDeviceState>();
    mSpeakerState = std::make_shared<OutputDeviceState>();
    ClientEventCallback = eventCallback;
    
    
    
}

void ATCRadioStack::resetRadioFx(unsigned int freq, bool except_click)
{
    if (!except_click) {
        mRadioState[freq].Click.reset();
        mRadioState[freq].mLastRxCount = 0;
    }
    mRadioState[freq].BlockTone.reset();
    mRadioState[freq].Crackle.reset();
}

void
ATCRadioStack::mix_buffers(audio::SampleType * RESTRICT src_dst, const audio::SampleType * RESTRICT src2, float src2_gain)
{
    for (int i = 0; i < audio::frameSizeSamples; i++) {
        src_dst[i] += (src2_gain * src2[i]);
    }
}

bool ATCRadioStack::mix_effect(std::shared_ptr<audio::ISampleSource> effect, float gain, std::shared_ptr<OutputDeviceState> state) {
    if (effect && gain > 0.0f) {
        auto rv = effect->getAudioFrame(state->mFetchBuffer);
        if (rv == audio::SourceStatus::OK) {
            ATCRadioStack::mix_buffers(state->mChannelBuffer, state->mFetchBuffer, gain);
        } else {
            return false;
        }
    }
    return true;
}

void ATCRadioStack::set_radio_effects(size_t rxIter, float crackleGain, float &whiteNoiseGain)
{
    whiteNoiseGain = fxWhiteNoiseGain;
    if (whiteNoiseGain > 0.0f) {
        if (!mRadioState[rxIter].WhiteNoise) {
            mRadioState[rxIter].WhiteNoise = std::make_shared<audio::PinkNoiseGenerator>();
        }
    }
    if (crackleGain > 0.0f) {
        if (!mRadioState[rxIter].Crackle) {
            mRadioState[rxIter].Crackle = std::make_shared<audio::RecordedSampleSource>(
                    mResources->mCrackle,
                    true);
        }
    }
}

bool ATCRadioStack::_process_radio(
        const std::map<void *, audio::SampleType[audio::frameSizeSamples]> &sampleCache,
        std::map<void *, audio::SampleType[audio::frameSizeSamples]> &eqSampleCache,
                                     size_t rxIter, std::shared_ptr<OutputDeviceState> state)
{
    ::memset(state->mChannelBuffer, 0, audio::frameSizeBytes);
    if (mPtt.load() && mRadioState[rxIter].tx) {
        // don't analyze and mix-in the radios transmitting, but suppress the
        // effects.
        resetRadioFx(rxIter);
        //AudiableAudioStreams[rxIter].store(0);
        return true;
    }
    // now, find all streams that this applies to.
    float crackleGain = 0.0f;
    uint32_t concurrentStreams = 0;
    for (auto &srcPair: mIncomingStreams) {
        if (!srcPair.second.source || !srcPair.second.source->isActive() ||
            (sampleCache.find(srcPair.second.source.get()) == sampleCache.end())) {
            continue;
        }
        bool mUseStream = false;
        float voiceGain = 1.0f;
        for (const afv::dto::RxTransceiver &tx: srcPair.second.transceivers) {
            if (tx.Frequency == mRadioState[rxIter].Frequency) {
                mUseStream = true;

                float crackleFactor = 0.0f;
                if (!mRadioState[rxIter].mBypassEffects) {
                    crackleFactor = static_cast<float>(
                            (exp(tx.DistanceRatio) * pow(tx.DistanceRatio, -2.5) / 350.0) - 0.00776652);
                    crackleFactor = fmax(0.0f, crackleFactor);
                    crackleFactor = fmin(0.15f, crackleFactor);

                    crackleGain += crackleFactor;
                }
                break; // matched once.  dont' bother anymore.
            }
        }
        if (mUseStream) {
            // then include this stream.
            try {
                mix_buffers(
                        state->mChannelBuffer,
                        sampleCache.at(srcPair.second.source.get()),
                        voiceGain * mRadioState[rxIter].Gain);
                concurrentStreams++;
            } catch (const std::out_of_range &) {
                LOG("ATCRadioStack", "internal error:  Tried to mix uncached stream");
            }
        }
    }
    //AudiableAudioStreams[rxIter].store(concurrentStreams);
    if (concurrentStreams > 0) {
        if(mRadioState[rxIter].mLastRxCount==0)
        {
            //Post Begin Voice Receiving Notfication
            unsigned int freq = rxIter;
            ClientEventCallback->invokeAll(ClientEventType::RxOpen, &freq);
        }
        if (!mRadioState[rxIter].mBypassEffects) {
            // if FX are enabled, and we muxed any streams, eq the buffer now to apply the bandwidth simulation,
            // but don't interfere with the effects.
            mRadioState[rxIter].vhfFilter.transformFrame(state->mChannelBuffer, state->mChannelBuffer);

            float whiteNoiseGain = 0.0f;
            set_radio_effects(rxIter, crackleGain, whiteNoiseGain);
            if (!mix_effect(mRadioState[rxIter].Crackle, crackleGain * mRadioState[rxIter].Gain,state)) {
                mRadioState[rxIter].Crackle.reset();
            }
            if (!mix_effect(mRadioState[rxIter].WhiteNoise, whiteNoiseGain * mRadioState[rxIter].Gain,state)) {
                mRadioState[rxIter].WhiteNoise.reset();
            }
        } // bypass effects
        if (concurrentStreams > 1) {
            if (!mRadioState[rxIter].BlockTone) {
                mRadioState[rxIter].BlockTone = std::make_shared<audio::SineToneSource>(fxBlockToneFreq);
            }
            if (!mix_effect(mRadioState[rxIter].BlockTone, fxBlockToneGain * mRadioState[rxIter].Gain,state)) {
                mRadioState[rxIter].BlockTone.reset();
            }
        } else {
            if (mRadioState[rxIter].BlockTone) {
                mRadioState[rxIter].BlockTone.reset();
            }
        }
    } else {
        resetRadioFx(rxIter, true);
        if (mRadioState[rxIter].mLastRxCount > 0) {
            mRadioState[rxIter].Click = std::make_shared<audio::RecordedSampleSource>(mResources->mClick, false);
            //Post End Voice Receiving Notification
            unsigned int freq = rxIter;
            ClientEventCallback->invokeAll(ClientEventType::RxClosed, &freq);
        }
    }
    mRadioState[rxIter].mLastRxCount = concurrentStreams;
    // if we have a pending click, play it.
    if (!mix_effect(mRadioState[rxIter].Click, fxClickGain * mRadioState[rxIter].Gain,state)) {
        mRadioState[rxIter].Click.reset();
    }
    // now, finally, mix the channel buffer into the mixing buffer.
    mix_buffers(state->mMixingBuffer, state->mChannelBuffer);
    return false;
}

audio::SourceStatus ATCRadioStack::getAudioFrame(audio::SampleType *bufferOut, bool onHeadset)
{
    
    std::shared_ptr<OutputDeviceState> state = onHeadset ? mHeadsetState: mSpeakerState;
    
    std::lock_guard<std::mutex> radioStateGuard(mRadioStateLock);
    std::lock_guard<std::mutex> streamGuard(mStreamMapLock);

    std::map<void *, audio::SampleType[audio::frameSizeSamples]> sampleCache;
    std::map<void *, audio::SampleType[audio::frameSizeSamples]> eqSampleCache;
    
    
    

    uint32_t allStreams = 0;
    // first, pull frames from all active audio sources.
    for (auto &src: mIncomingStreams) {
        
        unsigned int freq = src.second.transceivers.front().Frequency;
        if(freq==0) continue;
  
        bool match = mRadioState[freq].onHeadset == onHeadset;
        bool positiveRTOverride = (!onHeadset && mRadioState[freq].onHeadset && mRT);
        bool negativeRTOverride = (onHeadset && mRadioState[freq].onHeadset && mRT);
        
        if( positiveRTOverride || (match && !negativeRTOverride) )
        {
            if (src.second.source && src.second.source->isActive() &&
            (sampleCache.find(src.second.source.get()) == sampleCache.end())) {
                const auto rv = src.second.source->getAudioFrame(sampleCache[src.second.source.get()]);
                if (rv != audio::SourceStatus::OK) {
                    sampleCache.erase(src.second.source.get());
                } else {
                    allStreams++;
                                     
                    
                }
            }
        }
    }
    IncomingAudioStreams.store(allStreams);

    // empty the output buffer.
    ::memset(state->mMixingBuffer, 0, sizeof(audio::SampleType) * audio::frameSizeSamples);

    for (auto &radio: mRadioState)
    {
        bool match = radio.second.onHeadset == onHeadset;
        bool positiveRTOverride = (!onHeadset && radio.second.onHeadset && mRT);
        bool negativeRTOverride = (onHeadset && radio.second.onHeadset && mRT);
        
        if( positiveRTOverride || (match && !negativeRTOverride) )
        {
            _process_radio(sampleCache, eqSampleCache, radio.second.Frequency, state);
        }
    }

    ::memcpy(bufferOut, state->mMixingBuffer, sizeof(audio::SampleType) * audio::frameSizeSamples);
    return audio::SourceStatus::OK;
}

bool ATCRadioStack::_packetListening(const afv::dto::AudioRxOnTransceivers &pkt)
{
    //std::lock_guard<std::mutex> radioStateLock(mRadioStateLock);
    for (auto trans: pkt.Transceivers)
    {
        if(mRadioState[trans.Frequency].rx) {
            mRadioState[trans.Frequency].lastTransmitCallsign=pkt.Callsign;
            return true;
        }
        
    }
    
    return false;    
}


void ATCRadioStack::rxVoicePacket(const afv::dto::AudioRxOnTransceivers &pkt)
{
    std::lock_guard<std::mutex> streamMapLock(mStreamMapLock);
    
    
    //FIXME:  Deal with the case of a single-callsign transmitting multiple different voicestreams simultaneously.
    
    if(_packetListening(pkt))
    {
        mIncomingStreams[pkt.Callsign].source->appendAudioDTO(pkt);
        mIncomingStreams[pkt.Callsign].transceivers = pkt.Transceivers;
    }
}

void ATCRadioStack::setUDPChannel(cryptodto::UDPChannel *newChannel)
{
    if (mChannel != nullptr) {
        mChannel->unregisterDtoHandler("AR");
    }
    mChannel = newChannel;
    if (mChannel != nullptr) {
        mChannel->registerDtoHandler(
                "AR", [this](const unsigned char *data, size_t len) {
                    try {
                        dto::AudioRxOnTransceivers rxAudio;
                        auto objHdl = msgpack::unpack(reinterpret_cast<const char *>(data), len);
                        objHdl.get().convert(rxAudio);
                        this->rxVoicePacket(rxAudio);
                    } catch (const msgpack::type_error &e) {
                        LOG("atcradiostack", "unable to unpack audio data received: %s", e.what());
                        LOGDUMPHEX("radiosimulation", data, len);
                    }
                });
    }
}
void ATCRadioStack::setClientPosition(double lat, double lon, double amslm, double aglm)
{
    mClientLatitude = lat;
    mClientLongitude = lon;
    mClientAltitudeMSLM = amslm;
    mClientAltitudeGLM = aglm;
    
}

void ATCRadioStack::setTransceivers(unsigned int freq, std::vector<afv::dto::StationTransceiver> transceivers)
{
    // What we received is an array of StationTransceivers we got from the API.. we need to convert these to regular
    // Transceivers before we put them into the Radio State Object
    
    std::lock_guard<std::mutex> radioStateGuard(mRadioStateLock);
    mRadioState[freq].transceivers.clear();
    
    std::vector<dto::Transceiver> outTransceivers;
    for (auto inTrans: transceivers)
    {
        //Transceiver IDs all set to 0 here, they will be updated when coalesced into the global transceiver package
        dto::Transceiver out(0,freq,inTrans.LatDeg,inTrans.LonDeg,inTrans.HeightMslM,inTrans.HeightAglM);
        mRadioState[freq].transceivers.emplace_back(out);
    }
}

std::vector<afv::dto::Transceiver> ATCRadioStack::makeTransceiverDto()
{
    std::vector<afv::dto::Transceiver> retSet;
    unsigned int i=0;
    for (auto &state: mRadioState)
    {
        if(state.second.transceivers.size()==0)
        {
            //If there are no transceivers received from the network, we're using the client position
            retSet.emplace_back(i,state.first,mClientLatitude,mClientLongitude,mClientAltitudeMSLM, mClientAltitudeGLM);
            i++;
        }
        else
        {
            for (auto &trans: state.second.transceivers)
            {
                retSet.emplace_back(i,trans.Frequency,trans.LatDeg,trans.LonDeg,trans.HeightMslM, trans.HeightAglM);
                trans.ID=i;
                i++;
            }
        }
    }
    return std::move(retSet);
}


void ATCRadioStack::putAudioFrame(const audio::SampleType *bufferIn)
{
    if(mTick) mTick->tick();
    
    // do the peak/Vu calcs
    {
        auto *b = bufferIn;
        int i = audio::frameSizeSamples - 1;
        audio::SampleType peak = fabs(*(b++));
        while (i-- > 0) {
            peak = std::max<audio::SampleType>(peak, fabs(*(b++)));
        }
        double peakDb = 20.0 * log10(peak);
        peakDb = std::max(-40.0, peakDb);
        peakDb = std::min(0.0, peakDb);
        mVuMeter.addDatum(peakDb);
    }
    if (!mPtt.load() && !mLastFramePtt) {
        // Tick the sequence over when we have no Ptt as the compressed endpoint wont' get called to do that.
        std::atomic_fetch_add<uint32_t>(&mTxSequence, 1);
        return;
    }
    if (mVoiceFilter) {
        mVoiceFilter->putAudioFrame(bufferIn);
    } else {
        mVoiceSink->putAudioFrame(bufferIn);
    }
}

/** Audio enters here from the Codec Compressor before being sent out on to the network.
 *
 *
 *
 *
 */
void ATCRadioStack::processCompressedFrame(std::vector<unsigned char> compressedData)
{
    
    if (mChannel != nullptr && mChannel->isOpen()) {
        dto::AudioTxOnTransceivers audioOutDto;
        {
            std::lock_guard<std::mutex> radioStateGuard(mRadioStateLock);

            if (!mPtt.load()) {
                audioOutDto.LastPacket = true;
                mLastFramePtt = false;
            } else {
                mLastFramePtt = true;
            }
            
            for (auto &radio: mRadioState)
            {
                for(auto &trans : radio.second.transceivers)
                {
                    if(radio.second.tx) audioOutDto.Transceivers.emplace_back(trans.ID);
                }
            }
        }
        audioOutDto.SequenceCounter = std::atomic_fetch_add<uint32_t>(&mTxSequence, 1);
        audioOutDto.Callsign = mCallsign;
        audioOutDto.Audio = std::move(compressedData);
        mChannel->sendDto(audioOutDto);
    }
}


void ATCRadioStack::setPtt(bool pressed)
{
    mPtt.store(pressed);
}

void ATCRadioStack::setRT(bool active)
{
    mRT.store(active);
}

double ATCRadioStack::getVu() const
{
    return std::max(-40.0, mVuMeter.getAverage());
}

double ATCRadioStack::getPeak() const
{
    return std::max(-40.0, mVuMeter.getMax());
}
std::string ATCRadioStack::lastTransmitOnFreq(unsigned int freq)
{
    //std::lock_guard<std::mutex> mRadioStateGuard(mRadioStateLock);
    return mRadioState[freq].lastTransmitCallsign;
}
bool ATCRadioStack::getTxActive(unsigned int freq)
{
    std::lock_guard<std::mutex> mRadioStateGuard(mRadioStateLock);
    if(!mRadioState[freq].tx) return false;
    return mPtt.load();
}
bool ATCRadioStack::getRxActive(unsigned int freq)
{
    std::lock_guard<std::mutex> mRadioStateGuard(mRadioStateLock);
    return (mRadioState[freq].mLastRxCount >0);
}

void ATCRadioStack::addFrequency(unsigned int freq, bool onHeadset)
{
    std::lock_guard<std::mutex> mRadioStateGuard(mRadioStateLock);
    mRadioState[freq].Frequency=freq;
   
    mRadioState[freq].onHeadset=onHeadset;
    mRadioState[freq].tx=false;
    mRadioState[freq].rx=true;
    mRadioState[freq].mBypassEffects=false;
}

void ATCRadioStack::setCallsign(const std::string &newCallsign)
{
    mCallsign = newCallsign;
}

void ATCRadioStack::setGain(unsigned int freq, float gain)
{
    std::lock_guard<std::mutex> mRadioStateGuard(mRadioStateLock);
    mRadioState[freq].Gain=gain;
}

void ATCRadioStack::setTx(unsigned int freq, bool tx)
{
    std::lock_guard<std::mutex> mRadioStateGuard(mRadioStateLock);
    if (tx==false && mRadioState[freq].rx==false)
    {
        mRadioState.erase(freq);
        return;
    }
    if(mRadioState[freq].tx==tx) return;
    
    
    mRadioState[freq].tx=tx;
    mRadioState[freq].Frequency=freq;

    
    
}

void ATCRadioStack::setRx(unsigned int freq, bool rx)
{
    std::lock_guard<std::mutex> mRadioStateGuard(mRadioStateLock);
    if (rx==false && mRadioState[freq].tx==false)
    {
        mRadioState.erase(freq);
        return;
    }
    if(mRadioState[freq].rx==rx) return;
    mRadioState[freq].rx=rx;
    mRadioState[freq].Frequency=freq;
}

void ATCRadioStack::setOnHeadset(unsigned int freq, bool onHeadset)
{
    std::lock_guard<std::mutex> mRadioStateGuard(mRadioStateLock);
    mRadioState[freq].onHeadset=onHeadset;
}

void ATCRadioStack::reset()
{
    {
        std::lock_guard<std::mutex> ml(mStreamMapLock);
        mIncomingStreams.clear();
    }
    mTxSequence.store(0);
    mPtt.store(false);
    mLastFramePtt = false;
    // reset the voice compression codec state.
    mVoiceSink->reset();
}

void ATCRadioStack::maintainIncomingStreams()
{
    std::lock_guard<std::mutex> ml(mStreamMapLock);
    std::vector<std::string> callsignsToPurge;
    util::monotime_t now = util::monotime_get();
    for (const auto &streamPair: mIncomingStreams) {
        auto idleTime = now - streamPair.second.source->getLastActivityTime();
        if ((now - streamPair.second.source->getLastActivityTime()) > audio::compressedSourceCacheTimeoutMs) {
            callsignsToPurge.emplace_back(streamPair.first);
        }
    }
    for (const auto &callsign: callsignsToPurge) {
        mIncomingStreams.erase(callsign);
    }
    mMaintenanceTimer.enable(maintenanceTimerIntervalMs);
    
}

bool ATCRadioStack::getEnableInputFilters() const
{
    return static_cast<bool>(mVoiceFilter);
}

void ATCRadioStack::setEnableInputFilters(bool enableInputFilters)
{
    if (enableInputFilters) {
        if (!mVoiceFilter) {
            mVoiceFilter = std::make_shared<audio::SpeexPreprocessor>(mVoiceSink);
        }
    } else {
        mVoiceFilter.reset();
    }
}

void ATCRadioStack::setEnableOutputEffects(bool enableEffects)
{
    for (auto &thisRadio: mRadioState) {
        thisRadio.second.mBypassEffects = !enableEffects;
    }
}

void ATCRadioStack::setTick(std::shared_ptr<audio::ITick> tick)
{
    mTick = tick;
    
}

