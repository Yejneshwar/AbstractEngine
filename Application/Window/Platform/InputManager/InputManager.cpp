#include "InputManager.h"

namespace Application {
    
    // The static singleton instance definition.
    static InputManager s_Instance;
    
    InputManager& InputManager::Get() {
        return s_Instance;
    }
    
    // --- Static Updaters ---
    // These methods are called from the Objective-C++ layer (MetalView)
    // to update the state of the singleton.
    
    void InputManager::OnKeyPress(KeyCode key) {
        s_Instance.m_PressedKeys.insert(key);
    }
    
    void InputManager::OnKeyRelease(KeyCode key) {
        s_Instance.m_PressedKeys.erase(key);
    }
    
    void InputManager::OnTouchDown(float x, float y, size_t touchPoints) {
        if(touchPoints == 1)
            s_Instance.m_IsPrimaryButtonPressed = true;
        else
            s_Instance.m_IsSecondaryButtonPressed = true;
        s_Instance.m_PointerPosition = { x, y };
    }
    
    void InputManager::OnTouchUp(float x, float y) {
        s_Instance.m_IsPrimaryButtonPressed = false;
        s_Instance.m_IsSecondaryButtonPressed = false;
        s_Instance.m_PointerPosition = { x, y };
    }
    
    void InputManager::OnTouchMoved(float x, float y) {
        s_Instance.m_PointerPosition = { x, y };
    }
    
    // --- Polling Method Implementations ---
    
    bool InputManager::IsKeyPressed(KeyCode key) {
        return m_PressedKeys.count(key);
    }
    
    bool InputManager::IsMouseButtonPressed(MouseCode button) {
        // We'll map the primary touch to the left mouse button.
        if (button == Mouse::ButtonLeft) {
            return m_IsPrimaryButtonPressed;
        }
        return m_IsSecondaryButtonPressed;
    }
    
    glm::vec2 InputManager::GetMousePosition() {
        return m_PointerPosition;
    }
    
}
