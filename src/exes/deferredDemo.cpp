#include "engine/gl_deferred_eng.h"

int main(int argc, char* argv[]) {
    DeferredEngine engine;

    engine.init();

    engine.run();

    engine.cleanup();
    
    return 0;
}