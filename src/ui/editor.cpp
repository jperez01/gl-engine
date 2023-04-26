#include "editor.h"

void SceneEditor::render(Camera& camera)
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "Ctrl+N")) {

			}
			if (ImGui::MenuItem("Load", "Ctrl+L")) {

			}
			if (ImGui::MenuItem("Save", "Ctrl+S")) {

			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Add")) {
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	ImGui::BeginTabBar("Additional Parameters");
	if (ImGui::BeginTabItem("Entities")) {
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Render Options")) {
		renderer->handleImGui();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Stats")) {
		ImGui::EndTabItem();
	}
	ImGui::EndTabBar();

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	ImGui::Begin("Entity Properties", nullptr);
	ImGui::Text("Info");
	ImGui::End();
}

void SceneEditor::renderDebug(Camera& camera)
{
}
