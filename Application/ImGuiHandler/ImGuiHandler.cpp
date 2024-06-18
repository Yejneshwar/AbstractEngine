#include "ImGuiHandler.h"

void ImGuiHandler::NewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

inline void ImGuiHandler::Render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

ImGuiHandler::ImGuiHandler(GLFWwindow* window, const char* glsl_version) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls

#ifdef IMGUI_DOCKING_BRANCH_ENABLED
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigViewportsNoTaskBarIcon = true;
#endif

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();


    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void ImGuiHandler::Update(const ImGuiUpdateFn& updateFn) {
    this->NewFrame();

#ifdef IMGUI_DOCKING_BRANCH_ENABLED
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), dockspace_flags);
#endif

    updateFn();

    this->Render();
}
