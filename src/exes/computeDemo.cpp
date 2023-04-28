#include "core/application.h"
#include "engine/gl_compute_eng.h"

int main(int argc, char* argv[]) {
    ComputeEngine engine;
    Application app(&engine);

    app.init();

    app.mainLoop();

    app.cleanup();

    return 0;
}