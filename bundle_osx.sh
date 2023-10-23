#!/usr/bin/env bash

#
# Creating the macOs bundle
#

#
# Copying all files
#

rm -r build/VectorAudio.app

cd resources
sh make_icns.sh
cd ..

python3 collect_licenses.py

mkdir -p build/VectorAudio.app/Contents/{MacOS,Resources,Frameworks}
cp resources/*.wav build/VectorAudio.app/Contents/Resources
cp resources/*.ttf build/VectorAudio.app/Contents/Resources
cp resources/LICENSE.txt build/VectorAudio.app/Contents/Resources
cp resources/airports.json build/VectorAudio.app/Contents/Resources
cp resources/VectorAudio.icns build/VectorAudio.app/Contents/Resources
cp lib/macos/libafv_native.dylib build/VectorAudio.app/Contents/Frameworks
cp resources/icon_mac.png build/VectorAudio.app/Contents/Resources

chmod +x build/vector_audio.app/Contents/MacOS/vector_audio
chmod +x build/VectorAudio.app/Contents/Frameworks/libafv_native.dylib
cp build/vector_audio.app/Contents/MacOS/vector_audio build/VectorAudio.app/Contents/MacOS

cp resources/Info.plist build/VectorAudio.app/Contents/

install_name_tool -add_rpath "@executable_path/../Frameworks" build/VectorAudio.app/Contents/MacOS/vector_audio

rm resources/VectorAudio.icns

xattr -cr build/VectorAudio.app
codesign --force --timestamp -s - build/VectorAudio.app/Contents/Frameworks/libafv_native.dylib
codesign --force --deep --timestamp -s - build/VectorAudio.app