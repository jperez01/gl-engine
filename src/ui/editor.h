#pragma once
#include "utils/camera.h"
#include "engine/gl_base_engine.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"
#include "ImGuizmo.h"

class GLEngine;

class SceneEditor {
public:
	SceneEditor() = default;

	void render(Camera& camera);
	void renderAsList(Model& model);
	void renderDebug(Camera& camera);

	GLEngine* renderer = nullptr;
	std::vector<Model> *objs = nullptr;
	Mesh* chosenObj = nullptr;

private:
};