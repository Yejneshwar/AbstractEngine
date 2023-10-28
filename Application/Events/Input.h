#pragma once

#include "Codes/KeyCodes.h"
#include "Codes/MouseCodes.h"
#include <glm/glm.hpp>


namespace Application {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

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