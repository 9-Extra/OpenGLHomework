#include "engine/GlobalRuntime.h"
#include <PainterSystem.h>
#include <MoveSystem.h>
#include <sstream>

int main(int argc, char **argv) {
    try {
        
        engine_init();
        
        runtime->register_system(new PainterSystem());
        runtime->register_system(new MoveSystem());

        runtime->enter_loop();

        engine_shutdown();

        std::cout << "Normal exit!\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
