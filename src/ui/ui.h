#pragma once

#include "utils/gl_funcs.h"
#include "utils/camera.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"
#include "ImGuizmo.h"

namespace UI {
	extern ImGuizmo::OPERATION operation;

	void init();

	void drawIcon(int x, int y, float size = 0, float alpha = 1.0f);

	void inspectObject(glm::mat4 &matrix);
	bool manipulateMatrix(glm::mat4& matrix, Camera& camera);
};