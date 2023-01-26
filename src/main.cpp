#include "engine/gl_engine.h"

int main(int argc, char* argv[]) {
    GLEngine engine;

    engine.init();

    engine.run();

    engine.cleanup();
    
    return 0;
}