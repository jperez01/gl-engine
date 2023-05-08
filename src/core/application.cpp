#include "application.h"
#include "ui/ui.h"

Application::Application(GLEngine* renderer) {
    mRenderer = renderer;
}

void GLAPIENTRY
MessageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{

    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);

}

void Application::init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s \n", SDL_GetError());
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
        | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("GL Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, window_flags);
    if (window == nullptr) {
        printf("Error: %s \n", SDL_GetError());
        return;
    }

    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    gladLoadGLLoader(SDL_GL_GetProcAddress);
    SDL_GL_SetSwapInterval(1);

    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    for (int i = 0; i < numExtensions; i++) {
        std::string extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
        std::cout << extension << "\n";
        if (extension == "GL_ARB_bindless_texture") {
            std::cout << "Bindless Textures supported" << "\n";
            break;
        }
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 460");

    UI::init();

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    camera = Camera(glm::vec3(0.0f, 5.0f, 5.0f));
    camera.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

    mRenderer->camera = &camera;

    mRenderer->init_resources();

    // asyncLoadModel("../../resources/objects/sponzaBasic/glTF/Sponza.gltf", GLTF);

    Model newModel("../../resources/objects/sponzaBasic/glTF/Sponza.gltf", GLTF);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.1f));
    newModel.model_matrix = model;
    mRenderer->loadModelData(newModel);
    usableObjs.push_back(newModel);

    mRenderer->handleObjs(usableObjs);

    mEditor.renderer = mRenderer;
    mEditor.objs = &usableObjs;
}

void Application::cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Application::mainLoop()
{
    while (!closedWindow) {
        float currentFrame = static_cast<float>(SDL_GetTicks());
        deltaTime = currentFrame - lastFrame;
        deltaTime *= 0.01f;
        lastFrame = currentFrame;

        handleEvents();
        handleImportedObjs();

        mRenderer->render(usableObjs);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        mEditor.render(camera);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }
}

void Application::handleEvents()
{
    SDL_Event event;
    SDL_Keycode type;
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            closedWindow = true;
        }
        else if (event.type == SDL_KEYUP) {
            type = event.key.keysym.sym;
            if (type >= SDLK_RIGHT && type <= SDLK_UP) keyDown[type - SDLK_RIGHT] = false;
        }
        else if (event.type == SDL_KEYDOWN) {
            type = event.key.keysym.sym;
            if (type >= SDLK_RIGHT && type <= SDLK_UP) keyDown[type - SDLK_RIGHT] = true;

            if (type == SDLK_m) handleMouseMovement = !handleMouseMovement;
        }
        else if (event.type == SDL_MOUSEMOTION && (!io.WantCaptureMouse || ImGuizmo::IsOver())
            && handleMouseMovement) {
            mouse_callback(event.motion.x, event.motion.y);
        }
        else if (event.type == SDL_MOUSEWHEEL) {
            scroll_callback(event.wheel.y);
        }
        else if (event.type == SDL_WINDOWEVENT
            && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            framebuffer_callback(event.window.data1, event.window.data2);
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            handleClick(event.motion.x, event.motion.y);
        }
    }

    for (int i = 0; i < 4; i++) {
        type = keyDown[i] ? SDLK_RIGHT + i : 0;

        if (type == SDLK_UP)
            camera.processKeyboard(FORWARD, deltaTime);

        if (type == SDLK_DOWN)
            camera.processKeyboard(BACKWARD, deltaTime);

        if (type == SDLK_LEFT)
            camera.processKeyboard(LEFT, deltaTime);

        if (type == SDLK_RIGHT)
            camera.processKeyboard(RIGHT, deltaTime);
    }
}

void Application::handleImportedObjs()
{
    if (!importedObjs.empty()) {
        Model model = importedObjs[importedObjs.size() - 1];
        mRenderer->loadModelData(model);

        importedObjs.pop_back();
        usableObjs.push_back(model);
    }
}

void Application::asyncLoadModel(std::string path, FileType type)
{
    Model newModel(path, type);

    importedObjs.push_back(newModel);
}

void Application::mouse_callback(double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

void Application::scroll_callback(double yoffset)
{
    camera.processMouseScroll(static_cast<float>(yoffset));
}

void Application::framebuffer_callback(int width, int height)
{
    glViewport(0, 0, width, height);
    WINDOW_HEIGHT = height;
    WINDOW_WIDTH = width;
    mRenderer->WINDOW_HEIGHT = height;
    mRenderer->WINDOW_WIDTH = width;
}

void Application::handleClick(double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);

    float convertedX = (2.0f * xpos) / WINDOW_WIDTH - 1.0f;
    float convertedY = (2.0f * ypos) / WINDOW_HEIGHT - 1.0f;
    glm::vec4 ray_clip(convertedX, convertedY, -1.0f, 1.0f);
    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 1.0f);
    glm::vec4 ray_world = glm::inverse(camera.getViewMatrix()) * ray_eye;

    glm::vec4 ray_origin = glm::vec4(camera.Position.x, camera.Position.y, camera.Position.z, 1.0f);
    glm::vec4 ray_dir = glm::normalize(ray_world - ray_origin);
    glm::vec4 inverse_dir = glm::vec4(1.0f) / ray_dir;

    checkIntersection(ray_origin, ray_dir, inverse_dir);
}

void Application::checkIntersection(glm::vec4& origin, glm::vec4& direction, glm::vec4& inverse_dir)
{
    for (int i = 0; i < usableObjs.size(); i++) {
        glm::vec4 boxMin = usableObjs[i].model_matrix * usableObjs[i].aabb.minPoint;
        glm::vec4 boxMax = usableObjs[i].model_matrix * usableObjs[i].aabb.maxPoint;

        float tmin = -INFINITY, tmax = INFINITY;
        if (direction.x != 0.0f) {
            float t1 = (boxMin.x - origin.x) * inverse_dir.x;
            float t2 = (boxMax.x - origin.x) * inverse_dir.x;

            tmin = std::max(tmin, std::min(t1, t2));
            tmax = std::min(tmax, std::max(t1, t2));
        }

        if (direction.y != 0.0f) {
            float t1 = (boxMin.y - origin.y) * inverse_dir.y;
            float t2 = (boxMax.y - origin.y) * inverse_dir.y;

            tmin = std::max(tmin, std::min(t1, t2));
            tmax = std::min(tmax, std::max(t1, t2));
        }

        if (direction.z != 0.0f) {
            float t1 = (boxMin.z - origin.z) * inverse_dir.z;
            float t2 = (boxMax.z - origin.z) * inverse_dir.z;

            tmin = std::max(tmin, std::min(t1, t2));
            tmax = std::min(tmax, std::max(t1, t2));
        }

        if (tmax >= tmin) {
            chosenObjIndex = i;
            break;
        }
    }
}
