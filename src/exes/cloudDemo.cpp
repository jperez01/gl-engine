#include "core/application.h"
#include "engine/cloud_eng.h"

int main(int argc, char* argv[]) {
    CloudEngine engine;
    Application app(&engine);

    app.init();

    app.mainLoop();

    app.cleanup();

    return 0;
}