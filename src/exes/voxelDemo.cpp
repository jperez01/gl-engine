#include "core/application.h"
#include "engine/gl_voxel_eng.h"

int main(int argc, char* argv[]) {
    VoxelEngine engine;
    Application app(&engine);

    app.init();

    app.mainLoop();

    app.cleanup();

    return 0;
}