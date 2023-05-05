#include "engine/gl_indirect_eng.h"
#include "core/application.h"

int main(int argc, char* argv[]) {
    IndirectEngine engine;
    Application app(&engine);

    app.init();

    app.mainLoop();

    app.cleanup();

    return 0;
}