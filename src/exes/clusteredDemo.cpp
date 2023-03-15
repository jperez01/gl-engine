#include "engine/gl_clustered_engine.h"

int main(int argc, char* argv[]) {
    ClusteredEngine engine;

    engine.init();

    engine.run();

    engine.cleanup();
    
    return 0;
}