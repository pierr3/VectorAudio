#pragma once
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "AFVatcClientWrapper.h"
#include <thread>
#include <string>
#include <map>
#include <shared_mutex>
#include <memory>
#include "config.h"
#include <iostream>
#include "modals/settings.h"
#include "shared.h"

namespace afv_unix::application {
    class App {
        public:
            App();
            ~App();

            void render_frame();

        private:
            afv_native::api::atcClient* mClient;
            
        };
}