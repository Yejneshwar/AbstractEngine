#include "Input.h"
#include "AbstractApplication.h"
#include "Window/Platform/InputManager/InputManager.h"


bool Application::Input::IsKeyPressed(KeyCode key) {
    return InputManager::Get().IsKeyPressed(key);
}

bool Application::Input::IsMouseButtonPressed(MouseCode button) {
    return InputManager::Get().IsMouseButtonPressed(button);
}

glm::vec2 Application::Input::GetMousePosition() {
    return InputManager::Get().GetMousePosition();
}
