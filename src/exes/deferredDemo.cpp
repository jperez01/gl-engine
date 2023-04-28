#include "core/application.h"
#include "engine/gl_deferred_eng.h"

int main(int argc, char* argv[]) {
    DeferredEngine engine;
    Application app(&engine);

    app.init();

    app.mainLoop();

    app.cleanup();

    return 0;
}