//
//  SourceToSinkAdapter.h
//  afv_native
//
//  Created by Mike Evans on 11/18/20.
//

#ifndef SourceToSinkAdapter_h
#define SourceToSinkAdapter_h
#include "afv-native/audio/audio_params.h"
#include "afv-native/audio/ISampleSink.h"
#include "afv-native/audio/ISampleSource.h"

namespace afv_native {
    namespace audio {
    
    class SourceToSinkAdapter {
    public:
        SourceToSinkAdapter(std::shared_ptr<ISampleSource> inSource, std::shared_ptr<ISampleSink> inSink);
        ~SourceToSinkAdapter();
        void tick();
        
        
    private:
        audio::SampleType *mBuffer;
        std::shared_ptr<ISampleSource> mSource;
        std::shared_ptr<ISampleSink> mSink;
        
    };
    
    }
}


#endif /* SourceToSinkAdapter_h */
