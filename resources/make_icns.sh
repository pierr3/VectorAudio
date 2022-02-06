#!/usr/bin/env bash

mkdir VectorAudio.iconset
sips -z 16 16     icon_mac.png --out VectorAudio.iconset/icon_16x16.png
sips -z 32 32     icon_mac.png --out VectorAudio.iconset/icon_16x16@2x.png
sips -z 32 32     icon_mac.png --out VectorAudio.iconset/icon_32x32.png
sips -z 64 64     icon_mac.png --out VectorAudio.iconset/icon_32x32@2x.png
sips -z 128 128   icon_mac.png --out VectorAudio.iconset/icon_128x128.png
sips -z 256 256   icon_mac.png --out VectorAudio.iconset/icon_128x128@2x.png
sips -z 256 256   icon_mac.png --out VectorAudio.iconset/icon_256x256.png
sips -z 512 512   icon_mac.png --out VectorAudio.iconset/icon_256x256@2x.png
sips -z 512 512   icon_mac.png --out VectorAudio.iconset/icon_512x512.png
cp icon_mac.png VectorAudio.iconset/icon_512x512@2x.png
iconutil -c icns VectorAudio.iconset
rm -R VectorAudio.iconset