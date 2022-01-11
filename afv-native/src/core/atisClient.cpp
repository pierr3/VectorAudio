//
//  atisClient.cpp
//  afv_native
//
//  Created by Mike Evans on 11/18/20.
//

#include "afv-native/atisClient.h"


#include <memory>
#include <functional>

#include "afv-native/Log.h"
#include "afv-native/afv/params.h"
#include "afv-native/afv/VoiceSession.h"
#include "afv-native/afv/dto/voice_server/AudioTxOnTransceivers.h"
#include "afv-native/audio/RecordedSampleSource.h"
#include "afv-native/audio/WavFile.h"
#include "afv-native/audio/WavSampleStorage.h"

using namespace afv_native;
using namespace afv_native::afv;


ATISClient::ATISClient(
        struct event_base *evBase,
        std::string atisFile,
        const std::string &clientName,
        std::string baseUrl):
        mEvBase(evBase),
        mTransferManager(mEvBase),
        mVoiceSink(std::make_shared<VoiceCompressionSink>(*this)),
        mAPISession(mEvBase, mTransferManager, std::move(baseUrl), clientName),
        mVoiceSession(mAPISession),
        mClientLatitude(0.0),
        mClientLongitude(0.0),
        mClientAltitudeMSLM(100.0),
        mClientAltitudeGLM(100.0),
        mCallsign(),
        mTransceiverUpdateTimer(mEvBase, std::bind(&ATISClient::sendTransceiverUpdate, this)),
        mClientName(clientName),
        ClientEventCallback(),
        mATISFileName(atisFile),
        mChannel(&mVoiceSession.getUDPChannel()),
        looped(false),
        playCachedData(false)
        
    
{
    mAPISession.StateCallback.addCallback(this, std::bind(&ATISClient::sessionStateCallback, this, std::placeholders::_1));
   // mAPISession.AliasUpdateCallback.addCallback(this, std::bind(&ATISClient::aliasUpdateCallback, this));
    //mAPISession.StationTransceiversUpdateCallback.addCallback(this, std::bind(&ATISClient::stationTransceiversUpdateCallback, this));
    mVoiceSession.StateCallback.addCallback(this, std::bind(&ATISClient::voiceStateCallback, this, std::placeholders::_1));
}

ATISClient::~ATISClient()
{
    mVoiceSession.StateCallback.removeCallback(this);
    mAPISession.StateCallback.removeCallback(this);
    mAPISession.AliasUpdateCallback.removeCallback(this);

        
   
    
}

void ATISClient::setClientPosition(double lat, double lon, double amslm, double aglm)
{
    mClientLatitude = lat;
    mClientLongitude = lon;
    mClientAltitudeMSLM = amslm;
    mClientAltitudeGLM = aglm;
}


bool ATISClient::connect()
{
    if (!isAPIConnected()) {
        if (mAPISession.getState() != afv::APISessionState::Disconnected) {
            return false;
        }
        mAPISession.Connect();
    } else {
        mVoiceSession.Connect();
    }
    return true;
}

void ATISClient::disconnect()
{
    // voicesession must come first.
    if (isVoiceConnected()) {
        mVoiceSession.Disconnect(true);
    } else {
        mAPISession.Disconnect();
    }
}

void ATISClient::setCredentials(const std::string &username, const std::string &password)
{
    if (mAPISession.getState() != afv::APISessionState::Disconnected) {
        return;
    }
    mAPISession.setUsername(username);
    mAPISession.setPassword(password);
}

void ATISClient::setFrequency(unsigned int freq) {
    mFrequency=freq;
}

void ATISClient::setCallsign(std::string callsign)
{
    if (isVoiceConnected()) {
        return;
    }
    mVoiceSession.setCallsign(callsign);
   
    mCallsign = std::move(callsign);
}

void ATISClient::voiceStateCallback(afv::VoiceSessionState state)
{
    afv::VoiceSessionError voiceError;
    int channelErrno;

    switch (state) {
    case afv::VoiceSessionState::Connected:
        LOG("afv::ATISClient", "Voice Session Connected");
        
        queueTransceiverUpdate();
        ClientEventCallback.invokeAll(ClientEventType::VoiceServerConnected, nullptr);
        break;
    case afv::VoiceSessionState::Disconnected:
        LOG("afv::ATISClient", "Voice Session Disconnected");
        stopAudio();
        stopTransceiverUpdate();
        // bring down the API session too.
        mAPISession.Disconnect();
       
        ClientEventCallback.invokeAll(ClientEventType::VoiceServerDisconnected, nullptr);
        break;
    case afv::VoiceSessionState::Error:
        LOG("afv::ATISClient", "got error from voice session");
        stopAudio();
        stopTransceiverUpdate();
        // bring down the API session too.
        mAPISession.Disconnect();
       
        voiceError = mVoiceSession.getLastError();
        if (voiceError == afv::VoiceSessionError::UDPChannelError) {
            channelErrno = mVoiceSession.getUDPChannel().getLastErrno();
            ClientEventCallback.invokeAll(ClientEventType::VoiceServerChannelError, &channelErrno);
        } else {
            ClientEventCallback.invokeAll(ClientEventType::VoiceServerError, &voiceError);
        }
        break;
    }
}

void ATISClient::sessionStateCallback(afv::APISessionState state)
{
    afv::APISessionError sessionError;
    switch (state) {
    case afv::APISessionState::Reconnecting:
        LOG("afv_native::ATISClient", "Reconnecting API Session");
        break;
    case afv::APISessionState::Running:
        LOG("afv_native::ATISClient", "Connected to AFV API Server");
        if (!isVoiceConnected()) {
            mVoiceSession.setCallsign(mCallsign);
            mVoiceSession.Connect();
            mAPISession.updateStationAliases();
        }
        ClientEventCallback.invokeAll(ClientEventType::APIServerConnected, nullptr);
        break;
    case afv::APISessionState::Disconnected:
        LOG("afv_native::ATISClient", "Disconnected from AFV API Server.  Terminating sessions");
        // because we only ever commence a normal API Session teardown from a voicesession hook,
        // we don't need to call into voiceSession in this case only.
        ClientEventCallback.invokeAll(ClientEventType::APIServerDisconnected, nullptr);
        break;
    case afv::APISessionState::Error:
        LOG("afv_native::ATISClient", "Got error from AFV API Server.  Disconnecting session");
        sessionError = mAPISession.getLastError();
        ClientEventCallback.invokeAll(ClientEventType::APIServerError, &sessionError);
        break;
    default:
        // ignore the other transitions.
        break;
    }
}

void ATISClient::startAudio()
{
    looped=false;
    auto *wavData = audio::LoadWav(mATISFileName.c_str());
    if (nullptr == wavData) {
        LOG("ATISClient", "failed to load atis wavfile");
    } else {
        mWavSampleStorage = std::make_shared<audio::WavSampleStorage>(*wavData);
        
        mRecordedSampleSource = std::make_shared<audio::RecordedSampleSource>(mWavSampleStorage, true);

        mAdapter = std::make_shared<audio::SourceToSinkAdapter>(mRecordedSampleSource, shared_from_this());

        
    }

}

bool ATISClient::isPlaying() {
    
    return mAdapter!=nullptr;
    
}

void ATISClient::stopAudio()
{
    mAdapter=nullptr;
    mRecordedSampleSource.reset();
    mStoredData.clear();
    
   
    
}


void ATISClient::tick()
{
    
    
    if(playCachedData)
    {
        sendCachedFrame();
        return;
        
    }
    
    
    if(mAdapter)
    {
        mAdapter->tick();
    }
       
}
std::vector<afv::dto::Transceiver> ATISClient::makeTransceiverDto()
{
    std::vector<afv::dto::Transceiver> retSet;
    
        retSet.emplace_back(
                0, mFrequency, mClientLatitude,
                mClientLongitude, mClientAltitudeMSLM, mClientAltitudeGLM);
    
    return std::move(retSet);
}


void ATISClient::sendTransceiverUpdate()
{
    mTransceiverUpdateTimer.disable();
    if (!isAPIConnected() || !isVoiceConnected()) {
        return;
    }
    auto transceiverDto = makeTransceiverDto();
    

    /* ok - magic!
     *
     * so, in order to ensure that we flip the radio states to the CORRECT ONE
     * when the callback fires, we copy capture the update message itself (which is
     * all value copies) and use that to do the internal state update.
    */
    mVoiceSession.postTransceiverUpdate(
            transceiverDto,
            [this, transceiverDto](http::Request *r, bool success) {
                if (success && r->getStatusCode() == 200) {
                    
                    
                }
            });
    mTransceiverUpdateTimer.enable(afv::afvATCTransceiverUpdateIntervalMs);
}

void ATISClient::queueTransceiverUpdate()
{
    mTransceiverUpdateTimer.disable();
    if (!isAPIConnected() || !isVoiceConnected()) {
        return;
    }
    mTransceiverUpdateTimer.enable(0);
}



bool ATISClient::isAPIConnected() const
{
    auto sState = mAPISession.getState();
    return sState == afv::APISessionState::Running || sState == afv::APISessionState::Reconnecting;
}

bool ATISClient::isVoiceConnected() const
{
    return mVoiceSession.isConnected();
}

void ATISClient::setBaseUrl(std::string newUrl)
{
    mAPISession.setBaseUrl(std::move(newUrl));
}

void ATISClient::stopTransceiverUpdate()
{
    mTransceiverUpdateTimer.disable();
}


bool ATISClient::getEnableInputFilters() const
{
    return static_cast<bool>(mVoiceFilter);
}

double ATISClient::getInputPeak() const
{
//    if (mATCRadioStack) {
//        return mATCRadioStack->getPeak();
//    }
    return -INFINITY;
}

double ATISClient::getInputVu() const
{
//    if (mATCRadioStack) {
//        return mATCRadioStack->getVu();
//    }
    return -INFINITY;
}


void ATISClient::setEnableInputFilters(bool enableInputFilters)
{
    if (enableInputFilters) {
        if (!mVoiceFilter) {
            mVoiceFilter = std::make_shared<audio::SpeexPreprocessor>(mVoiceSink);
        }
    } else {
        mVoiceFilter.reset();
    }
}


void ATISClient::putAudioFrame(const audio::SampleType *bufferIn)
{
    
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
        
    }
    
    if (mVoiceFilter) {
        mVoiceFilter->putAudioFrame(bufferIn);
    } else {
        mVoiceSink->putAudioFrame(bufferIn);
    }
}

void ATISClient::sendCachedFrame() {
    if (mChannel != nullptr && mChannel->isOpen()) {
        dto::AudioTxOnTransceivers audioOutDto;
        {
            audioOutDto.Transceivers.emplace_back(0);
        }
        audioOutDto.SequenceCounter = std::atomic_fetch_add<uint32_t>(&mTxSequence, 1);
        audioOutDto.Callsign = mCallsign;
        audioOutDto.Audio = mStoredData[cacheNum];
        cacheNum++;
        if(cacheNum > mStoredData.size()) cacheNum=0;
        
        mChannel->sendDto(audioOutDto);
    }
    
    
}

/** Audio enters here from the Codec Compressor before being sent out on to the network.
 *
 *
 *
 *
 */
void ATISClient::processCompressedFrame(std::vector<unsigned char> compressedData)
{
    
    
    
    if(mRecordedSampleSource->firstFrame()) {
        if(looped) {
            
            printf("\nATIS LOOPED");
            playCachedData=true;
            cacheNum=0;
            return;
            
        }
        else {
            printf("\nATIS DATA INIT");
            
            looped=true;
            
        }
    }
    
    std::vector<unsigned char> temData;
    if(looped) {
        temData = compressedData;
        mStoredData.push_back(temData);
    }
    
    if (mChannel != nullptr && mChannel->isOpen()) {
        dto::AudioTxOnTransceivers audioOutDto;
        {
            audioOutDto.Transceivers.emplace_back(0);
        }
        audioOutDto.SequenceCounter = std::atomic_fetch_add<uint32_t>(&mTxSequence, 1);
        audioOutDto.Callsign = mCallsign;
        audioOutDto.Audio = std::move(compressedData);
        
        mChannel->sendDto(audioOutDto);
    }
}



