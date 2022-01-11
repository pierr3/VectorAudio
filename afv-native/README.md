# AFV Native

AFV Native is a portable-(ish) implementation of the Audio for VATSIM client protocol, written by Chris Collins.

For additional information, please see the [original repository](https://github.com/xsquawkbox/AFV-Native).

## Building

To build AFV-native, you will require an up-to-date copy of cmake and conan.
* [Conan](https://conan.io)
* [CMake](https://cmake.org)

```shell script
$ mkdir build && cd build
$ conan install ..
$ cmake ..
```

## Building on macOs

To build AFV-native, you will require an up-to-date copy of cmake, homebrew and conan.
* [Conan](https://conan.io)
* [CMake](https://cmake.org)
* [Brew](https://brew.sh)

```shell script
$ brew install speexdsp opus portaudio
$ mkdir build && cd build
$ conan install ..
$ cmake ..
```

## Licensing

AFV-Native is made available under the 3-Clause BSD License.  See `COPYING.md` for the precise licensing text.

More information about this is in the `docs` directory.