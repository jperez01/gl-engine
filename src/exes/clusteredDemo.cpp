#include "core/application.h"
#include "engine/gl_clustered_engine.h"

int main(int argc, char* argv[]) {
    ClusteredEngine engine;
    Application app(&engine);

    app.init();

    app.mainLoop();

    app.cleanup();

    return 0;
}