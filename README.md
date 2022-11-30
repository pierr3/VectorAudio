# Vector Audio
 A cross-platform Audio-For-VATSIM ATC Client for macOS and Linux, with untested Windows support (audio only)

 ![screengrab of application](https://i.imgur.com/IumERC2.png)

# Releases

See [releases](https://github.com/pierr3/VectorAudio/releases) for latest builds

# Usage

 - Open the application, fill in the settings page, then hit save.
 - Connect to VATSIM through EuroScope, wait till the client detects your connection, then click connect in VectorAudio.
 - Add the required frequencies through the right menu (CAUTION: Do not add two frequencies which are the same)
 - Use RX to receive, TX to transmit, XC to cross-couple
 - If anything acts up, delete the frequency using "X" and add it back

# Installation

## Linux

Download the latest release on the [release page](https://github.com/pierr3/VectorAudio/releases) and run the executable.

```sh
# Unzip the package
unzip VectorAudio-$VERSION-Ubuntu.zip

# cd into the newly created directory
cd VectorAudio-$VERSION-Ubuntu

# Make the file executable
chmod +x vector_audio

# Run it
./vector_audio
```
## macOS

Download the latest release on the [release page](https://github.com/pierr3/VectorAudio/releases) and install the .app into your applications folder.

Alternatively, VectorAudio can be installed using [Homebrew](https://brew.sh/index). Run the following commands to first install the Homebrew Tap and then the Homebrew Cask. This way the app gets upgraded when you run `brew upgrade`.

```sh
# Add the tap
brew tap flymia/homebrew-vectoraudio

# Install the cask
brew install --cask vectoraudio
```

VectorAudio ships with a universal binary, that includes x86_64 and ARM versions for Apple Silicon.

## Windows

Download the latest release on the [release page](https://github.com/pierr3/VectorAudio/releases) and run the executable. This should install VectorAudio.

# Build
## Dependencies

`cmake` and `pkg-config` should take care of this.

* OpenGL,
* SFML 2.5
* afv-native (atc-client branch) 
* imgui
* toml.hpp
* nlohmann.json

### imgui version

v1.86

## Build process

```sh
git submodule update --init --recursive
./vcpkg/bootstrap-vcpkg.sh -disableMetrics
mkdir -p build/ && cd build/
cmake .. && make
```

## Building on macOS

Be sure to have the packages `pkg-config` and `cmake` installed.

```sh
brew install cmake pkg-config
```

# Contributing

If you want to help with the project, you are always welcome to open a PR. 🙂
