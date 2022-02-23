#!/usr/bin/env bash

#
# Creating the linux bundle
#

#
# Copying all files
#


mkdir VectorAudio_Linux
cp resources/*.wav VectorAudio_Linux/
cp build/extern/afv-native/libafv.so VectorAudio_Linux/
cp LICENSE VectorAudio_Linux/

chmod +x build/vector_audio
cp build/vector_audio VectorAudio_Linux/

zip -r VectorAudio_Linux.zip VectorAudio_Linux/
