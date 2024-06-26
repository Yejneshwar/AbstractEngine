#pragma once
#ifndef IMGUIHANDLER_H
#define IMGUIHANDLER_H

//#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <functional>
#include "Core/Layer.h"

class ImGuiHandler : public GUI::Layer {
private:
    static void NewFrame();

    static void Render();

public:
    using ImGuiUpdateFn = std::function<void()>;

    ImGuiHandler(GLFWwindow* window, const char* glsl_version);

    void Update(const ImGuiUpdateFn& updateFn);

    ~ImGuiHandler() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void OnAttach() override;

    void OnDetach() override;

    void OnUpdateLayer() override;

    void OnDrawUpdate() override;

    void OnEvent(Application::Event& event) override;

    void OnSelection(int objectId, bool state) override;

    void OnImGuiRender() override;

};

#endif