# AFV-Unix
 An Audio-For-VATSIM ATC Client for macOs and Linux (audio only)

 ![screengrab of application](https://i.imgur.com/bFkTspr.png)

# Releases

See releases for test builds

# Usage

 - Open the application, fill in the settings page, then hit save.
 - Connect to VATSIM through EuroScope, wait till the client detects your connection, then click connect in VectorAudio.
 - Add the required frequencies through the right menu (CAUTION: Do not add two frequencies which are the same)
 - Use RX to receive, TX to transmit, XC to cross couple
 - If anything acts up, delete the frequency using "X" and add it back

# Dependencies

OpenGL, SFML 2.5, afv-native (atc-client branch), imgui, toml.hpp, nlohmann.json

# imgui version

v1.86