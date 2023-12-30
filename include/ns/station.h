#pragma once

#include <string>

namespace ns {
class Station {
public:
    [[nodiscard]] inline int getFrequencyHz() const { return pFrequencyHz; }
    [[nodiscard]] inline const std::string& getCallsign() const { return pCallsign; }
    [[nodiscard]] inline const std::string& getHumanFrequency() const { return pHumanFreq; }

    [[nodiscard]] inline int getTransceiverCount() const { return pTransceiverCount; }
    [[nodiscard]] inline bool hasTransceiver() const { return pTransceiverCount > 0; }
    inline void setTransceiverCount(int count) { pTransceiverCount = count; }

    inline static Station build(const std::string& callsign, int freqHz)
    {
        Station s;
        s.pCallsign = std::move(callsign);
        s.pFrequencyHz = freqHz;

        std::string temp = std::to_string(freqHz / 1000);
        s.pHumanFreq = temp.substr(0, 3) + "." + temp.substr(3, 7);

        return s;
    }

protected:
    int pFrequencyHz;
    std::string pCallsign;
    std::string pHumanFreq;

    int pTransceiverCount = -1;
};
}
