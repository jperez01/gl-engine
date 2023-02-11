#include "engine/gl_compute_eng.h"

int main(int argc, char* argv[]) {
    ComputeEngine engine;

    engine.init();

    engine.run();

    engine.cleanup();
    
    return 0;
}