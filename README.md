# Vector Audio
 A cross-platform Audio-For-VATSIM ATC Client for macOS, Windows and Linux support (audio only)

 ![screengrab of application](https://i.imgur.com/IumERC2.png)

## Releases

See [releases](https://github.com/pierr3/VectorAudio/releases) for latest builds

## FAQ

### My PTT does not work on macOS

macOS has strict permissioning around background keyboard inputs. VectorAudio should prompt you on first launch to allow it to monitor keyboard input. Sometimes, upon updating the app, this setting will undo itself. In that case, follow the steps described [in this issue](https://github.com/pierr3/VectorAudio/issues/30#issuecomment-1407573758).

### The station I am trying to add is not found

Ask your FE to define the station in the AFV database. Per the AFV FE manual, all stations should be defined in the database. VectorAudio does support ad-hoc station creation if you log-in as a DEL, GND or TWR that has no station definition. It will then place a transceiver at the location of the airport you logged in as. This will only work if the airport exists in the [airport database](https://github.com/mwgg/Airports/blob/master/airports.json?raw=true).

### Is there RDF support in EuroScope?

Yes! @KingfuChan has updated the RDF plugin for EuroScope to include support for VectorAudio. Find the plugin [in this repo](https://github.com/KingfuChan/RDF/).

### Does VectorAudio support HF Simulation?

No.

### I have an issue with VectorAudio

Read this document entirely first. If you can't find the answer to your problem, please [open an issue](https://github.com/pierr3/VectorAudio/issues/new) on GitHub, attaching relevant lines from the vector_audio.log file that should be in the same folder as the executable.

## Installation

### Linux

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

### macOS

Download the latest release on the [release page](https://github.com/pierr3/VectorAudio/releases) and install the .app into your applications folder.

Alternatively, VectorAudio can be installed using [Homebrew](https://brew.sh/index). Run the following commands to first install the Homebrew Tap and then the Homebrew Cask. This way the app gets upgraded when you run `brew upgrade`.

```sh
# Add the tap
brew tap flymia/homebrew-vectoraudio

# Install the cask
brew install --cask vectoraudio
```

VectorAudio ships with a universal binary, that includes x86_64 and ARM versions for Apple Silicon.

### Windows

Download the latest release on the [release page](https://github.com/pierr3/VectorAudio/releases) and run the executable. This should install VectorAudio.

## Build

### Dependencies

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

## Contributing

If you want to help with the project, you are always welcome to open a PR. 🙂
