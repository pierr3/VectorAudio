#include <atomic>
#include <thread>
#include <chrono>

#include <nlohmann/json.hpp>
#include <httplib.h>
#include "shared.h"

namespace afv_unix::data_file {
    // TODO Get the overarching status file to get data file url


    class Handler {
        public:
            inline Handler(){
                dataFileThread = std::thread(&Handler::_thread, this);
                dataFileThread.detach();
                spdlog::info("Created data file thread");
            };

            inline virtual ~Handler(){
                {
                    std::lock_guard<std::mutex> guard(this->df_m);
                    this->stop_flag = true;
                }
                this->df_cv.notify_all();

                if (dataFileThread.joinable())
                    dataFileThread.join();
            };

        private:
            const std::string data_feel_host = "https://data.vatsim.net";
            const std::string data_feel_url = "/v3/vatsim-data.json";
            
            std::thread dataFileThread;
            bool stop_flag = false;
            std::condition_variable df_cv;
            std::mutex df_m;

            inline void _thread() {

                httplib::Client cli(data_feel_host);
                using namespace std::chrono_literals;
                
                do {
                    
                    auto res = cli.Get(data_feel_url.c_str());
                    if (res) {
                        if (res->status == 200)
                        {
                            try {
                                auto j3 = nlohmann::json::parse(res->body);
                                if (j3["controllers"].is_array()) {
                                    
                                    bool connected_flag = false;
                                    bool atis_connected_flag = false;

                                    for(auto controller : j3["controllers"]) {
                                        if (controller["cid"] == afv_unix::shared::vatsim_cid) {
                                            
                                            afv_unix::shared::datafile::callsign = controller["callsign"].get<std::string>();
                                            afv_unix::shared::datafile::facility = controller["facility"].get<int>();
                                            afv_unix::shared::datafile::frequency = std::atof(controller["frequency"].get<std::string>().c_str())*1000000;
                                            afv_unix::shared::datafile::is_connected = true;
                                            connected_flag = true;
                                            break;
                                        }
                                    }

                                    for(auto atis : j3["atis"]) {
                                        if (atis["cid"] == afv_unix::shared::vatsim_cid) {
                                            
                                            afv_unix::shared::datafile::atis_callsign = atis["callsign"].get<std::string>();
                                            afv_unix::shared::datafile::atis_frequency = std::atof(atis["frequency"].get<std::string>().c_str())*1000000;
                                            if (atis["text_atis"].is_array()) {
                                                afv_unix::shared::datafile::atis_text.clear();
                                                for (auto atis_line : atis["text_atis"])
                                                    afv_unix::shared::datafile::atis_text.push_back(atis_line.get<std::string>());
                                            }
                                            atis_connected_flag = true;
                                            break;
                                        }
                                    }
                                    
                                    if (!connected_flag && afv_unix::shared::datafile::is_connected) {
                                        afv_unix::shared::datafile::is_connected = false;
                                        spdlog::info("Detected client disconnecting from network");
                                    }

                                    if (!atis_connected_flag && afv_unix::shared::datafile::is_atis_connected) {
                                        afv_unix::shared::datafile::is_atis_connected = false;
                                        afv_unix::shared::datafile::atis_callsign = "";
                                        afv_unix::shared::datafile::atis_frequency = 0;
                                        spdlog::info("Detected atis disconnecting from network");
                                    }
                                        
                                }
                            } catch (std::exception &exc) {
                                spdlog::error("Couldn't parse datafile json, error {}", exc.what());
                            }
                        }
                        else
                        {
                            spdlog::error("Couldn't download data file, error {}", res->status);
                        }
                    } else 
                    {
                        spdlog::warn("Couldn't open http request for data file, skipping");
                    }
                } while(this->wait_for(15s));

                spdlog::info("Data file thread terminated.");
            }

            template<class Duration>
            inline bool wait_for(Duration duration) {
                std::unique_lock<std::mutex> l(df_m);

                return !df_cv.wait_for(l, duration, [&]() { return stop_flag; });
            }
    };
}