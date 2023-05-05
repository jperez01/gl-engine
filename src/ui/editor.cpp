#include "editor.h"
#include "ui.h"


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
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

		ImGui::BeginChild("entities");

		if (objs != nullptr) {
			for (Model& model : *objs) {
				renderAsList(model);
			}
		}

		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();

		ImGui::EndChild();
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

	if (ImGui::Begin("Entity Properties")) {
		ImGui::Text("Info");
	}
	ImGui::End();

	if (ImGui::Begin("Gizmo")) {
		if (chosenObj != nullptr) {
			bool used = UI::manipulateMatrix(chosenObj->model_matrix, camera);
		}
	}
	ImGui::End();

	ImGui::ShowDemoWindow();
}

void SceneEditor::renderDebug(Camera& camera)
{
}

void SceneEditor::renderAsList(Model& model) {
	ImVec4 color(0.8f, 0.8f, 0.8f, 1.0f);

	bool open = ImGui::TreeNodeEx("##Model", ImGuiTreeNodeFlags_OpenOnArrow);
	ImGui::SameLine();
	UI::drawIcon(1, 5, 0, 1.0f);
	ImGui::SameLine();
	ImGui::Text("Model");
	ImGui::PushStyleColor(ImGuiCol_Text, color);

	if (open) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.65f, 0.8f, 1.0f));
		for (int i = 0; i < model.meshes.size(); i++) {
			bool isSelected = &model.meshes.at(i) == chosenObj;
			std::string name = "Mesh " + std::to_string(i);
			std::string itemId = "##" + std::to_string(i);

			if (ImGui::Selectable(itemId.c_str(), isSelected)) {
				chosenObj = &model.meshes.at(i);
			}
			ImGui::SameLine();
			ImGui::Text("-");
			ImGui::SameLine();
			UI::drawIcon(1, 9, 0, 1.0f);
			ImGui::SameLine();
			ImGui::Text(name.c_str());
		}
		ImGui::PopStyleColor();
		ImGui::TreePop();
	}

	ImGui::PopStyleColor();
	
}
