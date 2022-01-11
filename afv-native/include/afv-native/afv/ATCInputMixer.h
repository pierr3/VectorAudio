//
//  ATCInputMixer.h
//  afv_native
//
//  Created by Mike Evans on 10/22/20.
//

#ifndef ATCInputMixer_h
#define ATCInputMixer_h

#include "afv-native/audio/ISampleSink.h"
#include <map>


namespace afv_native {
    namespace afv {
    
    class ATCInputMixer;
    
    
    /** InputInPort - Routes Audio
     *
     *
     *
     */
    
    class InputInPort :
    public audio::ISampleSink,
    public std::enable_shared_from_this<InputInPort> {
    public:
        
        InputInPort(std::weak_ptr<ATCInputMixer> inMixer, unsigned int inPort);
        void putAudioFrame(const audio::SampleType *bufferIn, unsigned int inPort = 0 ) override;

        
    protected:
        std::weak_ptr<ATCInputMixer> mMixer;
        unsigned int mLocalPort;
        
    };
    
    
    
    
    /**   ATCInputMixer - Microphone to Network
     *
     *    Routes Audio from the Microphone(s) and eventually out onto the network.. this can get complicated if G/G Calls/Monitoring/etc are in progress.
     *
     *      Note: Mixer ports are NOT self routing by default.. You need to connect a port to itself if you want it to be a bus.
     *
     */
    
    class ATCInputMixer :
    public std::enable_shared_from_this<ATCInputMixer>{
        
    public:
        
        
        ATCInputMixer();
        virtual ~ATCInputMixer();

        /** attachInputDevice Returns an ISampleSink object to attach to the AudioDevice for input, connected to the mixer at the specified port
         *
         *  @return ISampleSink used to pass into the Audio Device as its sink
         *  @param port Mixer port to connect this input device to
         */
        std::shared_ptr<audio::ISampleSink> attachInputDevice(unsigned int port);
        
        
        /** attachOutput connects a port to the specified Sink
         *
         *  @param port Mixer Port to connect to
         *  @param sink Sink that will receive all audio flowing out this port
         *
         */
        
        void attachOutput(unsigned int port, std::shared_ptr<audio::ISampleSink> sink, unsigned int remotePort);
        
        
        /** makeMixerConnection connect two ports
         *
         *  @param srcport Port where the audio originates
         *  @param dstport Port where the audio is destined
         *  @param connect True to connect, False to Disconnect
         *
         *  */
        
        void makeMixerConnection(unsigned int srcport, unsigned int dstport, bool connect);
        
        
        
        bool hasMixerConnection(unsigned int srcport, unsigned int dstport);
        
        
        
        void putAudioFrame(const audio::SampleType *bufferIn, unsigned int inPort);
        
    protected:
        //               Output Port    Connected
        typedef std::map<unsigned int, bool> MixerMap;
        
        //                Input Port
        typedef std::map<unsigned int, MixerMap>   MixerTable;
        //                    Connection        Remote Port
        typedef std::pair<std::shared_ptr<audio::ISampleSink>, unsigned int> OutputSpec;
        //               Output Port
        typedef std::map<unsigned int, OutputSpec> OutputMap;
        
    
        
        
        
        MixerTable mMixer;
        
        //std::map<unsigned int,InputInPort> mInputs;
        std::map<unsigned int,std::shared_ptr<InputInPort>> mInputs;
        OutputMap mOutputs;
        
        
        
              
        
        
        
        
        
        
        
    };
    
    
    }
}


#endif /* ATCInputMixer_h */
