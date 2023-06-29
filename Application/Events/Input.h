#pragma once

#include "Codes/KeyCodes.h"
#include "Codes/MouseCodes.h"
#include "AbstractApplication.h"
#include <glm/glm.hpp>

namespace Application {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key) {
			auto* window = static_cast<GLFWwindow*>(GUI::AbstractApplication::Get().GetWindow().GetNativeWindow());
			auto state = glfwGetKey(window, static_cast<int32_t>(key));
			return state == GLFW_PRESS;
		}

		static bool IsMouseButtonPressed(MouseCode button) {
			auto* window = static_cast<GLFWwindow*>(GUI::AbstractApplication::Get().GetWindow().GetNativeWindow());
			auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
			return state == GLFW_PRESS;
		}

		static glm::vec2 GetMousePosition() {
			auto* window = static_cast<GLFWwindow*>(GUI::AbstractApplication::Get().GetWindow().GetNativeWindow());
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);

			return { (float)xpos, (float)ypos };
		}

		static float GetMouseX() {
			return GetMousePosition().x;
		}

		static float GetMouseY() {
			return GetMousePosition().y;
		}
	};
}