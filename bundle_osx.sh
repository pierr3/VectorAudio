#!/usr/bin/env bash

#
# Creating the macOs bundle
#

#
# Copying all files
#

rm -r build/Vector\ Audio.app

cd resources
sh make_icns.sh
cd ..

python collect_licenses.py

mkdir -p build/Vector\ Audio.app/Contents/{MacOS,Resources,Frameworks}
cp resources/*.wav build/Vector\ Audio.app/Contents/Resources
cp resources/*.ttf build/Vector\ Audio.app/Contents/Resources
cp resources/LICENSE.txt build/Vector\ Audio.app/Contents/Resources
cp resources/airports.json build/Vector\ Audio.app/Contents/Resources
cp resources/VectorAudio.icns build/Vector\ Audio.app/Contents/Resources
cp build/extern/afv-native/libafv.dylib build/Vector\ Audio.app/Contents/Frameworks
cp resources/icon_mac.png build/Vector\ Audio.app/Contents/Resources

chmod +x build/vector_audio
cp build/vector_audio build/Vector\ Audio.app/Contents/MacOS

cp resources/Info.plist build/Vector\ Audio.app/Contents/

install_name_tool -add_rpath "@executable_path/../Frameworks" build/Vector\ Audio.app/Contents/MacOS/vector_audio

rm resources/VectorAudio.icns

xattr -cr build/Vector\ Audio.app
codesign --force --deep -s - build/Vector\ Audio.app