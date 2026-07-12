#pragma once

#include "Codes/KeyCodes.h"
#include "Codes/MouseCodes.h"
#include <glm/glm.hpp>


namespace Application {

	// Platform-independent modifier query: raw key codes differ per backend
	// (macOS tracks kVK_* codes, GLFW tracks GLFW_KEY_*), so gestures and
	// camera bindings ask through this instead of a key code.
	enum class Modifier {
		Shift,
		Control,
		Option,  // Alt
		Command, // Super/Win
	};

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsModifierDown(Modifier modifier);

		static bool IsMouseButtonPressed(MouseCode button);

		static glm::vec2 GetMousePosition();

		static float GetMouseX() {
			return GetMousePosition().x;
		}

		static float GetMouseY() {
			return GetMousePosition().y;
		}
	};
}