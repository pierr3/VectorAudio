#ifndef SIMPLECOMPRESSOREFFECT_H
#define SIMPLECOMPRESSOREFFECT_H

#include <afv-native/audio/ISampleSource.h>

namespace chunkware_simple {
    class SimpleComp;
}

namespace afv_native::audio
{
    class SimpleCompressorEffect
    {
    public:
        explicit SimpleCompressorEffect();
        virtual ~SimpleCompressorEffect();

        void transformFrame(SampleType *bufferOut, SampleType const bufferIn[]);

    private:
        chunkware_simple::SimpleComp* m_simpleCompressor;
    };
}

#endif // SIMPLECOMPRESSOREFFECT_H
