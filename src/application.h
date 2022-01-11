#include "imgui.h"

namespace afv_unix::application {
    class App {
        public:
            App();
            virtual ~App();

            void render_frame();
    };
}