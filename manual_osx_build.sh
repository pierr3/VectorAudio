#!/usr/bin/env bash

version=$(cat VERSION)
build_type="Release"

mkdir build/
cmake -S . -B build_intel/ -DVCPKG_BUILD_TYPE=release -DCMAKE_BUILD_TYPE=$build_type -DCMAKE_OSX_ARCHITECTURES=x86_64 -DVCPKG_TARGET_TRIPLET=x64-osx
cmake --build build_intel/ --config $build_type
cp build_intel/vector_audio.app/Contents/MacOS/vector_audio build/vector_audio.intel
rm -rf build_intel/
cmake -S . -B build_arm64/ -DVCPKG_BUILD_TYPE=release -DCMAKE_BUILD_TYPE=$build_type -DCMAKE_OSX_ARCHITECTURES=arm64 -DVCPKG_TARGET_TRIPLET=arm64-osx
cmake --build build_arm64/ --config Release
cp build_arm64/vector_audio.app/Contents/MacOS/vector_audio build/vector_audio.arm
rm -rf build_arm64/
lipo -create build/vector_audio.arm build/vector_audio.intel -output build/vector_audio
./bundle_osx.sh
brew install create-dmg
create-dmg --volname "Vector Audio Installer" --app-drop-link 600 185 --window-size 800 400 --icon "VectorAudio.app" 200 190 "VectorAudio-$version-Universal.dmg" "build/VectorAudio.app"
codesign --force --deep --timestamp -s "Developer ID Application" VectorAudio-$version-Universal.dmg
