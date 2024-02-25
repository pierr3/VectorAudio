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
cp resources/icon_mac.png build/VectorAudio.app/Contents/Resources

rm resources/VectorAudio.icns

if [ $# -eq 1 ]; then
  chmod +x build/vector_audio.app/Contents/MacOS/vector_audio
  cp build/vector_audio.app/Contents/MacOS/vector_audio build/VectorAudio.app/Contents/MacOS
else
  chmod +x build/vector_audio
  cp build/vector_audio build/VectorAudio.app/Contents/MacOS
fi

cp resources/Info.plist build/VectorAudio.app/Contents/

chmod +x lib/macos/libafv_native.dylib
lipo -create lib/macos/libafv_native.dylib -output resources/libafv_native.framework/libafv_native
install_name_tool -change @rpath/libafv_native.dylib @loader_path/../Frameworks/libafv_native.framework/libafv_native build/VectorAudio.app/Contents/MacOS/vector_audio
cp -R resources/libafv_native.framework/ build/VectorAudio.app/Contents/Frameworks/libafv_native.framework

xattr -cr build/VectorAudio.app
if [ $# -eq 1 ]; then
  codesign --timestamp -s "Developer ID Application" build/VectorAudio.app
fi
