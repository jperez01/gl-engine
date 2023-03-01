#include "engine/gl_pbr_engine.h"

int main(int argc, char* argv[]) {
    PBREngine engine;

    engine.init();

    engine.run();

    engine.cleanup();
    
    return 0;
}