#!/usr/bin/env bash

#
# Creating the linux bundle
#

libname=$1

python3 collect_licenses.py

cp -R ./resources/VectorAudio.AppDir/ ./build/
cp ./resources/*.wav ./build/VectorAudio.AppDir/usr/share/vectoraudio/
cp ./resources/*.ttf ./build/VectorAudio.AppDir/usr/share/vectoraudio/
cp ./resources/LICENSE.txt ./build/VectorAudio.AppDir/usr/share/vectoraudio/
cp ./resources/airports.json ./build/VectorAudio.AppDir/usr/share/vectoraudio/
cp ./resources/icon_mac.png ./build/VectorAudio.AppDir/vectoraudio.png
cp ./resources/icon_mac.png ./build/VectorAudio.AppDir/.DirIcon
cp ./resources/icon_mac.png ./build/VectorAudio.AppDir/usr/share/vectoraudio/

cp ./lib/linux/$libname ./build/VectorAudio.AppDir/usr/lib/
chmod +x ./build/VectorAudio.AppDir/usr/lib/$libname

cp ./build/vector_audio ./build/VectorAudio.AppDir/usr/bin
chmod +x ./build/VectorAudio.AppDir/usr/bin/vector_audio
chmod 755 ./build/VectorAudio.AppDir/usr/bin/vector_audio
chmod +x ./build/VectorAudio.AppDir/vectoraudio.desktop
chmod +x ./build/VectorAudio.AppDir/AppRun

wget -O appimagetool-x86_64.AppImage https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x appimagetool-x86_64.AppImage

./appimagetool-x86_64.AppImage ./build/VectorAudio.AppDir -u 'gh-releases-zsync|pierr3|VectorAudio|latest|VectorAudio-*x86_64.AppImage.zsync'