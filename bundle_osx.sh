#!/usr/bin/env bash

#
# Creating the macOs bundle
#

#
# Copying all files
#

mkdir -p build/VectorAudio.app/Contents/{MacOS,Resources}
cp resources/*.wav build/VectorAudio.app/Contents/Resources
cp resources/VectorAudio.icns build/VectorAudio.app/Contents/Resources
cp build/libafv.dylib build/VectorAudio.app/Contents/MacOS

chmod +x build/afv_unix
cp build/afv_unix build/VectorAudio.app/Contents/MacOS

cat > build/VectorAudio.app/Contents/Info.plist << EOF
{
	CFBundleName = "vectoraudio";
	CFBundleDisplayName = "Vector Audio";
	CFBundleIndentifier = "org.vector.audio";
	CFBundleVersion = "1";
	CFBundleShortVersionString = "1";
	CFBundleInfoDictionaryVersion = "6.0";
	CFBundlePackageType = "APPL";
	CFBundleExecutable = "afv_unix";
	CFBundleIconFile = "VectorAudio.icns";
    NSHighResolutionCapable = "false";
}
EOF

arch=$(arch)

cd build/
zip -r "VectorAudio_$arch.zip" VectorAudio.app
