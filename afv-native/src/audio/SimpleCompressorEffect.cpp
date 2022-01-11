#include "afv-native/audio/SimpleCompressorEffect.h"
#include <SimpleComp.h>

using namespace afv_native::audio;

SimpleCompressorEffect::SimpleCompressorEffect() :
    m_simpleCompressor(new chunkware_simple::SimpleComp())
{
    m_simpleCompressor->setAttack(5.0);
    m_simpleCompressor->setRelease(10.0);
    m_simpleCompressor->setSampleRate(sampleRateHz);
    m_simpleCompressor->setThresh(16.0);
    m_simpleCompressor->setRatio(6.0);
    m_simpleCompressor->setMakeUpGain(-5.5);
    m_simpleCompressor->initRuntime();
}

SimpleCompressorEffect::~SimpleCompressorEffect()
{
    delete m_simpleCompressor;
}

void SimpleCompressorEffect::transformFrame(SampleType *bufferOut, const SampleType bufferIn[])
{
    for(int i = 0; i < frameSizeSamples; i++)
    {
        double in1 = bufferIn[i];
        double in2 = in1;
        m_simpleCompressor->process(in1, in2);
        bufferOut[i] = static_cast<SampleType>(in1);
    }
}
