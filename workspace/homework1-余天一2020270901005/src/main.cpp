#include "engine/GlobalRuntime.h"
#include <PainterSystem.h>
#include <MoveSystem.h>
#include <sstream>

bool event_loop() {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) {
            return true;
        }
    }
    return false;
}

int main(int argc, char **argv) {
    try {
        
        engine_init();
        
        runtime->register_system(new PainterSystem());
        runtime->register_system(new MoveSystem());

        runtime->logic_clock.update();

        while (!event_loop()) {
            runtime->tick();
        }

        engine_shutdown();

        std::cout << "Normal exit!\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
