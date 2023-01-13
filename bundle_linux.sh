#!/usr/bin/env bash

#
# Creating the linux bundle
#

python collect_licenses.py

mkdir VectorAudio_Ubuntu
mkdir VectorAudio_Ubuntu/lib
cp resources/*.wav VectorAudio_Ubuntu/
cp resources/*.ttf VectorAudio_Ubuntu/
cp build/extern/afv-native/libafv.so VectorAudio_Ubuntu/lib
cp resources/LICENSE.txt VectorAudio_Ubuntu/
cp resources/icon_mac.png VectorAudio_Ubuntu

chmod +x build/vector_audio
patchelf --set-rpath \$ORIGIN:\$ORIGIN/../lib build/vector_audio

cp build/vector_audio VectorAudio_Ubuntu/

tar -zcvf VectorAudio_Ubuntu.tar.gz VectorAudio_Ubuntu/
