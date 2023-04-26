#include "core/application.h"
#include "engine/gl_engine.h"

int main(int argc, char* argv[]) {
    RenderEngine engine;
    Application app(&engine);

    app.init();

    app.mainLoop();

    app.cleanup();

    return 0;
}