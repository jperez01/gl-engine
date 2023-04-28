#include "ui.h"

namespace UI {
    Texture icons;
    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;

    void init()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();
        icons = glutil::loadSomeTexture("../../resources/textures/icons.png");
    }

    void drawIcon(int x, int y, float size, float alpha)
    {
        if (icons.id == -1)
            return;

        if (size == 0)
            size = 16;

        float aspect = icons.width / icons.height;
        ImVec4 color(1.0f, 1.0f, 1.0f, alpha);
        ImVec2 uv0(x / 16.0f, x / 16.0f);
        ImVec2 uv1(uv0.x + 1.0f / 16.0f, uv0.y + 1.0f / 16.0f);

        glBindTextureUnit(0, icons.id);
        glTextureParameteri(icons.id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        ImGui::Image((void*)icons.id, ImVec2(size / aspect, size), uv0, uv1, color);
    }

    void inspectObject(glm::mat4& matrix)
    {
        float translation[3], rotation[3], scale[3];

        ImGuizmo::DecomposeMatrixToComponents(&matrix[0][0], translation, rotation, scale);
        ImGui::DragFloat3("Position", translation, 0.1f);
        ImGui::DragFloat3("Rotation", rotation, 0.1f);
        ImGui::DragFloat3("Scale", scale, 0.1f);
        ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, &matrix[0][0]);
    }

    bool manipulateMatrix(glm::mat4& matrix, Camera& camera)
    {
        if (ImGui::RadioButton("Translate", operation == ImGuizmo::TRANSLATE)) {
            operation = ImGuizmo::TRANSLATE;
        }
        if (ImGui::RadioButton("Rotate", operation == ImGuizmo::ROTATE)) {
            operation = ImGuizmo::ROTATE;
        }
        if (ImGui::RadioButton("Scale", operation == ImGuizmo::SCALE)) {
            operation = ImGuizmo::SCALE;
        }
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix();

        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        bool changed = ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
            operation, ImGuizmo::LOCAL, glm::value_ptr(matrix));

        return ImGuizmo::IsUsing();
    }
};