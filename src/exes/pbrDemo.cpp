#include "core/application.h"
#include "engine/gl_pbr_engine.h"

int main(int argc, char* argv[]) {
    PBREngine engine;
    Application app(&engine);

    app.init();

    app.mainLoop();

    app.cleanup();

    return 0;
}