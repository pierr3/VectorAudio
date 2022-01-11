from conans import ConanFile, CMake, tools

class AfvNativeConan(ConanFile):
    name = "AFV-Native"
    version = "1.2.0"
    license = "3-Clause BSD"
    author = "Chris Collins <kuroneko@sysadninjas.net>"
    url = "https://github.com/xpilot-project/AFV-Native"
    description = "Portable, Native Implementation of the AFV Interface"
    topics = ("vatsim", "afv", "voice", "portable")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "audio_library": ["portaudio", "soundio"],
        "build_examples": [True, False],
        "build_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "audio_library": "portaudio",
        "build_examples": False,
        "build_tests": False,
        "*:shared": False,
        "*:fPIC": True,
        "libcurl:with_ssl": "openssl",
        "libevent:with_openssl": False,
        "libsoundio*:enable_jack": False,
        "libsoundio*:enable_pulseaudio": True,
        "libsoundio*:enable_alsa": True,
    }
    generators = "cmake"
    requires = [
        "msgpack/3.3.0",
        "nlohmann_json/3.9.1",
        "openssl/1.1.1m",
        "libcurl/7.75.0",
        "libevent/2.1.12"
    ]
    build_requires = [
    ]
    exports_sources = [
        "docs/*",
        "!docs/api/*",
        "examples/*",
        "extern/*",
        "!extern/*/.git",
        "include/*",
        "src/*",
        "test/*",
        "CMakeLists.txt",
        "Doxyfile",
        "README.md",
        "COPYING.md",
        "!*/imgui.ini",
        "!*/afv.log",
    ]

    def imports(self):
        #self.copy("*.dll", "bin", "bin")
        #self.copy("*.dylib", "lib", "lib")
        #self.copy("*.so*", "lib", "lib")
        if self.settings.build_type == 'Debug':
            self.copy("*.pdb", "lib", "bin")

    def configure(self):
        pass

    def build_requirements(self):
        if self.options.build_examples:
            self.build_requires("glew/2.2.0rc2@xsquawkbox/devel")
            self.build_requires("sdl2/[~2.0.9]@bincrafters/stable")
        if self.options.build_tests:
            self.build_requires("gtest/[~1.8.1]")

    def source(self):
        pass

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure(source_folder=".")
        cmake.definitions["AFV_NATIVE_AUDIO_LIBRARY"] = self.options.audio_library
        cmake.definitions["BUILD_EXAMPLES"] = self.options.build_examples
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["afv_native"]
        if self.settings.compiler == 'Visual Studio':
            self.cpp_info.defines += ["_USE_MATH_DEFINES"]