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

                    if (res->status == 200)
                    {
                        try {
                            auto j3 = nlohmann::json::parse(res->body);
                            if (j3["controllers"].is_array()) {
                                
                                bool connected_flag = false;

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

                                        break;
                                        //TODO: Handle ATIS disconnect
                                    }
                                }
                                
                                if (!connected_flag)
                                    afv_unix::shared::datafile::is_connected = false;
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
                } while(this->wait_for(15s));
            }

            template<class Duration>
            inline bool wait_for(Duration duration) {
                std::unique_lock<std::mutex> l(df_m);

                return !df_cv.wait_for(l, duration, [&]() { return stop_flag; });
            }
    };
}