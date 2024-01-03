# VectorAudio

A cross-platform Audio-For-VATSIM ATC Client for macOS, Windows and Linux support (audio only)

![screengrab of application](https://raw.githubusercontent.com/pierr3/VectorAudio/main/splash.png)

## Releases

See [releases](https://github.com/pierr3/VectorAudio/releases) for latest builds

## FAQ

### My PTT does not work on macOS

macOS has strict permissioning around background keyboard inputs. VectorAudio should prompt you on first launch to allow it to monitor keyboard input. Sometimes, upon updating the app, this setting will undo itself. In that case, follow the steps described [in this issue](https://github.com/pierr3/VectorAudio/issues/30#issuecomment-1407573758).

### There is a console window appearing when I launch VectorAudio

Version 1.3.1 accidentally triggered this issue. A hotfix has been released for windows, version 1.3.1a which is available to download under the 1.3.1 release, install it and the issue will be resolved.

### Where is the configuration file/log file stored?

On macOS: `~/Library/Application\ Support/VectorAudio`
On Linux: `~/.config/vector_audio`
On Windows: where VectorAudio.exe is installed

### Does VectorAudio support joystick PTT?

Yes.

### The station I am trying to add is not found

Ask your FE to define the station in the AFV database. Per the AFV FE manual, all stations should be defined in the database. VectorAudio does support ad-hoc station creation if you log-in as a DEL, GND or TWR that has no station definition. It will then place a transceiver at the location of the airport you logged in as. This will only work if the airport exists in the [airport database](https://github.com/mwgg/Airports/blob/master/airports.json?raw=true).

### Is there RDF support in EuroScope?

Yes! @KingfuChan has updated the RDF plugin for EuroScope to include support for VectorAudio. Find the plugin [in this repo](https://github.com/KingfuChan/RDF/).

### What is this green "Slurper" text/yellow "Datafile" text/red "No VATSIM data"

VectorAudio uses two different ways to detect a connection to vatsim: the slurper and the datafile. If you see a yellow "datafile" label in the top right, that means the slurper is not available.

If you see a red "No VATSIM data", this most likely means that VATSIM servers are temporarily down.

The status of available endpoints is refreshed every 15 minutes.

### Does VectorAudio support HF Simulation?

No.

### Can I add a frequency manually if it does not exist in the database?

The AFV facility manager manual is clear that ALL stations must be added. If your vACC has not done so, please ensure they do. VectorAudio follows the database definition of frequencies and does not allow for the addition of custom frequencies at this moment.

### Can I extend VectorAudio using a plugin/is there an SDK?

Yes! Have a look [in the wiki](https://github.com/pierr3/VectorAudio/wiki/Using-the-SDK). If you need additional features, please open an issue with a detailed request, I'll be happy to look at it with no guarantees.

### I have an issue with VectorAudio

Read this document entirely first. If you can't find the answer to your problem, please [open an issue](https://github.com/pierr3/VectorAudio/issues/new) on GitHub, attaching relevant lines from the vector_audio.log file that should be in the same folder as the executable.

## Installation

### Linux

VectorAudio is packaged as an AppImage and should run without any specific actions.

Download the latest release on the [release page](https://github.com/pierr3/VectorAudio/releases) and run the AppImage file.
If it does not open, you might want to make sure it has permission to run as an executable by running `chmod +x` on the AppImage File.

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

VectorAudio uses OpenGL as a rendering backend, and thus requires an OpenGL compatible device.

`cmake` is required to build the project. Dependencies will be downloaded through vcpkg at build time. See vcpkg.json for further details

On linux, the following packages are required: `build-essentials libx11-dev libxrandr-dev libxi-dev libudev-dev libgl1-mesa-dev libxcursor-dev freeglut3-dev`, you may also need further packages to enable the different audio backends, such as Alsa, JACK or PulseAudio.
On macOS, XCode Command Line tools, CMake and Homebrew are required and the following homebrew package is required: `pkg-config`

## Build process

```sh
git submodule update --init --recursive
./vcpkg/bootstrap-vcpkg.sh -disableMetrics
mkdir -p build/ && cd build/
cmake .. && make
```

## Contributing

If you want to help with the project, you are always welcome to open a PR. 🙂
