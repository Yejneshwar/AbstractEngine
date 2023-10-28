#include "Input.h"
#include "AbstractApplication.h"


bool Application::Input::IsKeyPressed(KeyCode key) {
	auto* window = static_cast<GLFWwindow*>(GUI::AbstractApplication::Get().GetWindow().GetNativeWindow());
	auto state = glfwGetKey(window, static_cast<int32_t>(key));
	return state == GLFW_PRESS;
}

bool Application::Input::IsMouseButtonPressed(MouseCode button) {
	auto* window = static_cast<GLFWwindow*>(GUI::AbstractApplication::Get().GetWindow().GetNativeWindow());
	auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
	return state == GLFW_PRESS;
}

glm::vec2 Application::Input::GetMousePosition() {
	auto* window = static_cast<GLFWwindow*>(GUI::AbstractApplication::Get().GetWindow().GetNativeWindow());
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	return { (float)xpos, (float)ypos };
}
