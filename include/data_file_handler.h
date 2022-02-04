#include <atomic>
#include <thread>
#include <chrono>

#include <nlohmann/json.hpp>
#include "shared.h"

#include <SFML/Network.hpp>

namespace afv_unix::data_file {
    // I know this is "bad" TODO
    // TODO: Add Https handler, SFML does not support https
    const std::string data_feel_host = "http://data.vatsim.net/";
    const std::string data_feel_url = "v3/vatsim-data.json";
    
    inline std::atomic<bool> stop_flag = false;

    inline void handler() {

        using namespace std::chrono_literals;
        while(!stop_flag) {
            
            sf::Http http(data_feel_host);
            sf::Http::Request request(data_feel_url, sf::Http::Request::Get);
            sf::Http::Response response = http.sendRequest(request);

            if (response.getStatus() == sf::Http::Response::Ok)
            {
                try {
                    auto j3 = nlohmann::json::parse(response.getBody());
                    if (j3["controllers"].is_array()) {
                        
                        // Reset the flag so we can auto disconnect
                        afv_unix::shared::datafile::is_connected = false;

                        for(auto controller : j3["controllers"]) {

                            if (controller["cid"] == afv_unix::shared::vatsim_cid) {
                                
                                afv_unix::shared::datafile::callsign = controller["callsign"].get<std::string>();
                                afv_unix::shared::datafile::facility = controller["facility"].get<int>();
                                afv_unix::shared::datafile::frequency = std::atof(controller["frequency"].get<std::string>().c_str())*1000000;
                                afv_unix::shared::datafile::is_connected = true;
                                return;
                            }
                        }
                    }
                } catch (std::exception exc) {
                    //TODO: Handle error
                }
            }
            else
            {
                std::cout << "data file request failed" << std::endl;
                //TODO Add logging
            }

            std::this_thread::sleep_for(15s);
        }
    }
}