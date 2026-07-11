#include "3DCamera.h"
#include <iostream>
#include <Events/Input.h>
#include <Logger.h>





Graphics::ThreeDCamera::ThreeDCamera(float fov, float aspectRatio, float nearClip, float farClip)
	: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip))
{
	UpdateView();
}

void Graphics::ThreeDCamera::OnEvent(Application::Event& event)
{
	Application::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Application::MouseScrolledEvent>(APP_BIND_EVENT_FN(Graphics::ThreeDCamera::OnMouseScroll));
	dispatcher.Dispatch<Application::MouseMovedEvent>(APP_BIND_EVENT_FN(Graphics::ThreeDCamera::OnMouseMove));
	dispatcher.Dispatch<Application::MouseButtonPressedEvent>(APP_BIND_EVENT_FN(Graphics::ThreeDCamera::OnMousePressed));
	dispatcher.Dispatch<Application::PinchGestureEvent>(APP_BIND_EVENT_FN(Graphics::ThreeDCamera::OnPinch));
	dispatcher.Dispatch<Application::RotateGestureEvent>(APP_BIND_EVENT_FN(Graphics::ThreeDCamera::OnRotateGesture));
	dispatcher.Dispatch<Application::PanGestureEvent>(APP_BIND_EVENT_FN(Graphics::ThreeDCamera::OnPanGesture));
}

// Re-anchor the drag origin whenever a press/touch begins. Without this, the
// first move of a new touch computes its delta against the END of the
// previous drag (fingers teleport between touches, unlike a mouse) and the
// orbit jumps instead of continuing from where it left off.
bool Graphics::ThreeDCamera::OnMousePressed(Application::MouseButtonPressedEvent& e)
{
	m_InitialMousePosition = Application::Input::GetMousePosition();
	// Arm the drag threshold: camera manipulation only starts once the
	// pointer travels a few points, so a tap (especially an Apple Pencil
	// tap, which micro-jitters) selects without nudging the view.
	m_PressAnchor = m_InitialMousePosition;
	m_DragLatched = false;
	return false;
}

// Pinch = dolly zoom. Scale delta ~1.0; the offset maps to the same units as
// wheel zoom (yOffset * 0.1) with a feel-matched sensitivity.
bool Graphics::ThreeDCamera::OnPinch(Application::PinchGestureEvent& e)
{
	MouseZoom((e.GetScaleDelta() - 1.0f) * 5.0f);
	UpdateView();
	return false;
}

// Two-finger twist = orbit around the up axis.
bool Graphics::ThreeDCamera::OnRotateGesture(Application::RotateGestureEvent& e)
{
	float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
	m_Yaw += yawSign * e.GetAngleDelta();
	UpdateView();
	return false;
}

// Two-finger drag = pan, matching the scaling of mouse-move panning.
bool Graphics::ThreeDCamera::OnPanGesture(Application::PanGestureEvent& e)
{
	MousePan(glm::vec2(e.GetDeltaX(), e.GetDeltaY()) * 0.003f);
	UpdateView();
	return false;
}

glm::vec3 Graphics::ThreeDCamera::GetUpDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Graphics::ThreeDCamera::GetRightDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Graphics::ThreeDCamera::GetForwardDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::quat Graphics::ThreeDCamera::GetOrientation() const
{
	return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
}

glm::vec3 Graphics::ThreeDCamera::GetViewDirection() const
{
	// Calculate the view direction from the camera position
	return glm::normalize(m_Position - glm::vec3(m_ViewMatrix[3]));
}

void Graphics::ThreeDCamera::UpdateProjection()
{
	m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
	m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
#if BUILDING_METAL
//    Flip Y-axis to match coordinate system
    m_Projection[1][1] *= -1.0f;
#endif
}

void Graphics::ThreeDCamera::UpdateView()
{
	// m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
	m_Position = CalculatePosition();

	glm::quat orientation = GetOrientation();
	m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
	m_ViewMatrix = glm::inverse(m_ViewMatrix);
}


bool Graphics::ThreeDCamera::OnMouseScroll(Application::MouseScrolledEvent& e)
{
	float delta = e.GetYOffset() * 0.1f;
	MouseZoom(delta);
	UpdateView();
	return false;
}

bool Graphics::ThreeDCamera::OnMouseMove(Application::MouseMovedEvent& e)
{
	const glm::vec2& mouse(Application::Input::GetMousePosition());

	glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
	m_InitialMousePosition = mouse;

	// NOTE: the old iOS "discard large deltas" hack is gone — the platform
	// layer now tracks a single primary touch continuously, so deltas are
	// coherent, and multi-finger input arrives as gesture events instead.

	// Tap-vs-drag hysteresis: ignore movement until it exceeds the slop
	// radius from the press anchor (see OnMousePressed).
	if (!m_DragLatched) {
		constexpr float kDragThresholdPoints = 4.0f;
		if (glm::length(mouse - m_PressAnchor) < kDragThresholdPoints)
			return false;
		m_DragLatched = true;
	}

	if (Application::Input::IsMouseButtonPressed(Application::Mouse::ButtonLeft))
		MouseRotate(delta);
	else if (Application::Input::IsMouseButtonPressed(Application::Mouse::ButtonRight))
		MousePan(delta);

	UpdateView();
	return false;
}

void Graphics::ThreeDCamera::MousePan(const glm::vec2& delta)
{
	auto [xSpeed, ySpeed] = PanSpeed();
	m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
	m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
}

void Graphics::ThreeDCamera::MouseRotate(const glm::vec2& delta)
{
	float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
	m_Yaw += yawSign * delta.x * RotationSpeed();
	m_Pitch += delta.y * RotationSpeed();
}

void Graphics::ThreeDCamera::MouseZoom(float delta)
{
	m_Distance -= delta * ZoomSpeed();
	if (m_Distance < 0.01f)
	{
		m_FocalPoint += GetForwardDirection();
		m_Distance = 1.0f;
	}
}

glm::vec3 Graphics::ThreeDCamera::CalculatePosition() const
{
	return m_FocalPoint - GetForwardDirection() * m_Distance;
}

std::pair<float, float> Graphics::ThreeDCamera::PanSpeed() const
{
	float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
	float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

	float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
	float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

	return { xFactor, yFactor };
}

float Graphics::ThreeDCamera::RotationSpeed() const
{
	return 0.8f;
}

float Graphics::ThreeDCamera::ZoomSpeed() const
{
	float distance = m_Distance * 0.2f;
	distance = std::max(distance, 0.0f);
	float speed = distance * distance;
	speed = std::min(speed, 100.0f); // max speed = 100
	return speed;
}
