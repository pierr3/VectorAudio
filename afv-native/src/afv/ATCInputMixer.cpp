//
//  ATCInputMixer.cpp
//  afv_native
//
//  Created by Mike Evans on 10/22/20.
//



#include "afv-native/afv/ATCInputMixer.h"
#include "afv-native/afv/ATCRadioStack.h"
#include "afv-native/afv/RadioSimulation.h"

using namespace afv_native::afv;
using namespace afv_native;
using namespace afv_native::audio;


InputInPort::InputInPort(std::weak_ptr<ATCInputMixer> inMixer, unsigned int localPort) :
    mMixer(inMixer),
    mLocalPort(localPort)
{
    
}

void InputInPort::putAudioFrame(const SampleType *bufferIn, unsigned int inPort)
{
    if(auto mix = mMixer.lock()){
        mix->putAudioFrame(bufferIn,inPort);
    }
}





ATCInputMixer::ATCInputMixer() :
    mInputs(),
    mOutputs()

{
    
}

ATCInputMixer::~ATCInputMixer()
{
    
    
}

std::shared_ptr<audio::ISampleSink> ATCInputMixer::attachInputDevice(unsigned int port)
{
    auto iter = mInputs.find(port);
    if(iter == mInputs.end())
    {
        mInputs.erase(port);
    }
    
    std::shared_ptr<InputInPort> inPortObj = std::make_shared<InputInPort>(shared_from_this(),port);
    mInputs[port]=inPortObj;
    return inPortObj;
    
}

void ATCInputMixer::attachOutput(unsigned int localOutputPort, std::shared_ptr<audio::ISampleSink> sink, unsigned int remoteInputPort)
{
    OutputSpec out = OutputSpec(sink,remoteInputPort);
    mOutputs[localOutputPort]=out;
}

bool ATCInputMixer::hasMixerConnection(unsigned int srcport, unsigned int dstport)
{
    MixerTable::iterator iter = mMixer.find(srcport);
    if(iter==mMixer.end())
        return false;
    MixerMap::iterator iter2 = iter->second.find(dstport);
    if(iter2==iter->second.end())
        return false;
    return iter2->second;
    
}

void ATCInputMixer::makeMixerConnection(unsigned int srcport, unsigned int dstport, bool connect)
{
    MixerTable::iterator iter = mMixer.find(srcport);
    if(iter==mMixer.end())
    {
        //Port Doesn't exist in local map.. create entry & add
        MixerMap temMap;
        temMap[dstport]=connect;
        mMixer[srcport]=temMap;
        
    }
    else
    {
        //Already exists
        MixerMap::iterator iter2 = iter->second.find(dstport);
        if(iter2==iter->second.end())
        {
            //Output isn't present yet, add it:
            iter->second.insert(MixerMap::value_type(dstport,connect));
            
        }
        else
        {
            //Output is present.. update value
            iter2->second = connect;
        }
    }
}


void ATCInputMixer::putAudioFrame(const audio::SampleType *bufferIn, unsigned int dstPort)
{
    //Incoming Audio gets Routed here with port it's coming in on.
    

}






