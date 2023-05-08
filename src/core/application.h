#pragma once

#include "engine/gl_base_engine.h"

class Application {
public:
    Application(GLEngine *renderer);
	void init();
    void cleanup();
    void mainLoop();

private:
    void handleEvents();
    void handleImportedObjs();

    void mouse_callback(double xposIn, double yposIn);
    void scroll_callback(double yoffset);
    void framebuffer_callback(int width, int height);

    void handleClick(double xposIn, double yposIn);
    void checkIntersection(glm::vec4& origin, glm::vec4& direction, glm::vec4& inverse_dir);

    void asyncLoadModel(std::string path, FileType type = OBJ);

	GLEngine* mRenderer;
    SceneEditor mEditor;

    SDL_Window* window;
    SDL_GLContext gl_context;
    int WINDOW_WIDTH = 1920, WINDOW_HEIGHT = 1080;

    bool closedWindow = false;
    bool keyDown[4] = { false, false, false, false };

    float lastX = WINDOW_WIDTH / 2.0f;
    float lastY = WINDOW_HEIGHT / 2.0f;
    bool firstMouse = true;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    std::vector<Model> importedObjs;
    std::vector<Model> usableObjs;
    int chosenObjIndex = 0;
    ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;

    Camera camera;
    bool handleMouseMovement = true;

    float animationTime = 0.0f;
    int chosenAnimation = 0;
};