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

mkdir -p build/VectorAudio.app/Contents/{MacOS,Resources,lib}
cp resources/*.wav build/VectorAudio.app/Contents/Resources
cp resources/VectorAudio.icns build/VectorAudio.app/Contents/Resources
cp build/extern/afv-native/libafv.dylib build/VectorAudio.app/Contents/lib

chmod +x build/vector_audio
cp build/vector_audio build/VectorAudio.app/Contents/MacOS

cat > build/VectorAudio.app/Contents/Info.plist << EOF
{
	CFBundleName = "vectoraudio";
	CFBundleDisplayName = "Vector Audio";
	CFBundleIndentifier = "org.vector.audio";
	CFBundleVersion = "1";
	CFBundleShortVersionString = "1";
	CFBundleInfoDictionaryVersion = "6.0";
	CFBundlePackageType = "APPL";
	CFBundleExecutable = "vector_audio";
	CFBundleIconFile = "VectorAudio.icns";
    NSHighResolutionCapable = "false";
}
EOF

install_name_tool -add_rpath "@executable_path/../lib" build/VectorAudio.app/Contents/MacOS/vector_audio

rm resources/VectorAudio.icns