#include "Input.h"
#include "AbstractApplication.h"
#include "Window/Platform/InputManager/InputManager.h"


bool Application::Input::IsKeyPressed(KeyCode key) {
    return InputManager::Get().IsKeyPressed(key);
}

bool Application::Input::IsModifierDown(Modifier modifier) {
#ifdef __APPLE__
    // Apple window layers track modifiers with raw kVK_* codes
    // (see MacOSWindow flagsChanged / IOSWindow key handling).
    switch (modifier) {
        case Modifier::Shift:   return InputManager::Get().IsKeyPressed(56);
        case Modifier::Control: return InputManager::Get().IsKeyPressed(59);
        case Modifier::Option:  return InputManager::Get().IsKeyPressed(58);
        case Modifier::Command: return InputManager::Get().IsKeyPressed(55);
    }
#else
    // GLFW key codes (left/right variants).
    switch (modifier) {
        case Modifier::Shift:   return InputManager::Get().IsKeyPressed(340) || InputManager::Get().IsKeyPressed(344);
        case Modifier::Control: return InputManager::Get().IsKeyPressed(341) || InputManager::Get().IsKeyPressed(345);
        case Modifier::Option:  return InputManager::Get().IsKeyPressed(342) || InputManager::Get().IsKeyPressed(346);
        case Modifier::Command: return InputManager::Get().IsKeyPressed(343) || InputManager::Get().IsKeyPressed(347);
    }
#endif
    return false;
}

bool Application::Input::IsMouseButtonPressed(MouseCode button) {
    return InputManager::Get().IsMouseButtonPressed(button);
}

glm::vec2 Application::Input::GetMousePosition() {
    return InputManager::Get().GetMousePosition();
}
