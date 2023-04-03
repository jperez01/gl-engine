#include "engine/gl_voxel_eng.h"

int main(int argc, char* argv[]) {
    VoxelEngine engine;

    engine.init();

    engine.run();

    engine.cleanup();
    
    return 0;
}