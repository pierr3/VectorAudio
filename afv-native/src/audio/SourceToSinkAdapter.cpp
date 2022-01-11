//
//  SourceToSinkAdapter.cpp
//  afv_native
//
//  Created by Mike Evans on 11/18/20.
//

#include <memory>

#include "afv-native/audio/SourceToSinkAdapter.h"

using namespace afv_native::audio;
using namespace std;

SourceToSinkAdapter::SourceToSinkAdapter(std::shared_ptr<ISampleSource> inSource, std::shared_ptr<ISampleSink> inSink) :
    mSink(std::move(inSink)),
    mSource(std::move(inSource))
          
{
    mBuffer = new audio::SampleType[audio::frameSizeSamples];
}

SourceToSinkAdapter::~SourceToSinkAdapter()
{
    delete[] mBuffer;
    
}


void SourceToSinkAdapter::tick()
{
    mSource->getAudioFrame(mBuffer);
    mSink->putAudioFrame(mBuffer);    
}
