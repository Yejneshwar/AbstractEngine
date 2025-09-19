#pragma once
#ifndef IMGUIHANDLER_H
#define IMGUIHANDLER_H

#include <imgui.h>
#include <functional>
#include "Core/Layer.h"

class ImGuiHandler : public GUI::Layer {
private:
    static void NewFrame();

    static void Render();

public:
    using ImGuiUpdateFn = std::function<void()>;

    ImGuiHandler(void* window, const char* glsl_version);

    void Update(const ImGuiUpdateFn& updateFn);

    ~ImGuiHandler();

    void OnAttach() override;

    void OnDetach() override;

    void OnUpdateLayer() override;

    void OnDrawUpdate() override;

    void OnEvent(Application::Event& event) override;

    void OnSelection(int objectId, bool state) override;

    void OnImGuiRender() override;

};

#endif