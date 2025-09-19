#pragma once

#include <unordered_set>
#include "glm/glm.hpp"
#include "Events/Input.h"

#include "Events/EventTypes/ApplicationEvent.h"
#include "Events/EventTypes/MouseEvent.h"
#include "Events/EventTypes/KeyEvent.h"

namespace Application {

/**
 * @class InputManager
 * @brief A C++ singleton class to manage and poll input states.
 *
 * This class provides a platform-agnostic interface for checking the
 * current state of the keyboard and mouse/touch inputs. Its state is updated
 * by the platform-specific windowing layer (e.g., MetalView on iOS) via
 * the static `On...` methods.
 */
class InputManager {
public:
    /// Returns the singleton instance of the InputManager.
    static InputManager& Get();

    // --- Public Polling Methods ---
    bool IsKeyPressed(KeyCode key);
    bool IsMouseButtonPressed(MouseCode button);
    glm::vec2 GetMousePosition();

    // --- Static Updaters (called from platform layer) ---
    static void OnKeyPress(KeyCode key);
    static void OnKeyRelease(KeyCode key);
    static void OnTouchDown(float x, float y, size_t touchPoints);
    static void OnTouchUp(float x, float y);
    static void OnTouchMoved(float x, float y);

    InputManager() = default;
    ~InputManager() = default;
private:
    
    // Prevent copying and assignment
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    std::unordered_set<KeyCode> m_PressedKeys;
    bool m_IsPrimaryButtonPressed = false;
    bool m_IsSecondaryButtonPressed = false;
    glm::vec2 m_PointerPosition = { 0.0f, 0.0f };
};

}
