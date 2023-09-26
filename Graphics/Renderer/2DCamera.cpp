#include "2DCamera.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include <Events/Input.h>
#include <Logger.h>

#define LABEL_PIXELS 80

class GridCalc {
public:
	double base = 10;
	std::array<double, 3> major = { 5, 5, 5 };
	std::array<double, 3> minor = { 10, 10, 10 };
};

double s(double screenWidth, double vXmax, double vXmin) {
	return (LABEL_PIXELS / screenWidth) * (vXmax - vXmin);
}


double n(double t, GridCalc e, float& majorStep, float& minorStep) {
	double n = std::numeric_limits<double>::infinity();
	double a = std::numeric_limits<double>::infinity();
	double o = e.base;
	// get size of e.major
	auto i = e.major;
	auto r = e.minor;

	for (
		int s = 0;
		s < i.size();
		s++
		) {
		double m = i[s],
			S = ceil(log(t / m) / log(o)),
			p = m * pow(o, S);
		if (p < n) {
			a = (n = p) / r[s];
		}
	}


	LOG_TRACE_STREAM << "major Spacing: " << n << " minor spacing: " << a;
	majorStep = n;
	minorStep = a;
	return n;
}


void GetSpacing(double viewPortWidth, double xMin, double xMax, float& gridMajor, float& gridMinor) {
	LOG_TRACE_STREAM << xMin << " " << xMax;
	double sVal = s(viewPortWidth, xMax, xMin);
	n(sVal, GridCalc(), gridMajor, gridMinor);
}

Graphics::TwoDCamera::TwoDCamera(float nearClip = -100.0f, float farClip = 100.0f)
	: m_NearClip(nearClip), m_FarClip(farClip), Camera(glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, nearClip, farClip))
{
	UpdateView();
}

void Graphics::TwoDCamera::OnUpdate()
{
	bool mLeft = Application::Input::IsMouseButtonPressed(Application::Mouse::ButtonLeft);
	bool mRight = Application::Input::IsMouseButtonPressed(Application::Mouse::ButtonRight);
	const glm::vec2& mouse{ Application::Input::GetMouseX(), Application::Input::GetMouseY() };
	glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
	m_InitialMousePosition = mouse;

	if (!mLeft && !mRight) return;


	if (mLeft)
		MouseRotate(delta);
	else if (mRight)
		MousePan(delta);

	UpdateProjection();
	UpdateView();
}

void Graphics::TwoDCamera::OnEvent(Application::Event& event)
{
	Application::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Application::MouseScrolledEvent>(APP_BIND_EVENT_FN(Graphics::TwoDCamera::OnMouseScroll));
}

glm::vec3 Graphics::TwoDCamera::GetUpDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Graphics::TwoDCamera::GetRightDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Graphics::TwoDCamera::GetForwardDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::quat Graphics::TwoDCamera::GetOrientation() const
{
	return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
}

glm::vec3 Graphics::TwoDCamera::GetViewDirection() const
{
	// Calculate the view direction from the camera position
	return glm::normalize(m_Position - glm::vec3(m_ViewMatrix[3]));
}

void Graphics::TwoDCamera::UpdateProjection()
{
	float n = m_zoom; // Align 14 units horizontally in the screen.

	const float aspectRatio = m_ViewportWidth / m_ViewportHeight;
	m_AspectRatio = aspectRatio;
	float left = -n * 1.0f, right = n * 1.0f, down = -n * 1.0f / aspectRatio, up = n * 1.0f / aspectRatio;

	this->worldXmin = left;
	this->worldXmax = right;
	this->worldYmin = down;
	this->worldYmax = up;

	m_Projection = glm::ortho(left, right, down, up, m_NearClip, m_FarClip);

}

void Graphics::TwoDCamera::UpdateView()
{

	m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
	m_Position = CalculatePosition();

	//glm::quat orientation = GetOrientation();
	//m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
	//m_ViewMatrix = glm::inverse(m_ViewMatrix);


	glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position);

	m_ViewMatrix = glm::inverse(transform);
}


bool Graphics::TwoDCamera::OnMouseScroll(Application::MouseScrolledEvent& e)
{
	float delta = e.GetYOffset() * 0.1f;
	MouseZoom(delta);
	UpdateProjection();
	UpdateView();
	GetSpacing(m_ViewportWidth, worldXmin, worldXmax, gridMajorSpacing, gridMinorSpacing);
	return false;
}

void Graphics::TwoDCamera::MousePan(const glm::vec2& delta)
{
	auto [xSpeed, ySpeed] = PanSpeed();
	m_FocalPoint += -GetRightDirection() * delta.x * m_Distance;
	m_FocalPoint += GetUpDirection() * delta.y * m_Distance;
}

void Graphics::TwoDCamera::MouseRotate(const glm::vec2& delta)
{
	float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
	m_Yaw += yawSign * delta.x * RotationSpeed();
	m_Pitch += delta.y * RotationSpeed();
}

void Graphics::TwoDCamera::MouseZoom(float delta)
{
	//m_Distance -= delta * ZoomSpeed();
	if ((m_zoom - (delta * m_zoomLevel)) <= 0.0) {
		m_zoomLevel /= 10;
	}
	m_zoom -= delta * m_zoomLevel;

	//if (m_Distance < 0.01f)
	//{
	//	m_FocalPoint += GetForwardDirection();
	//	m_Distance = 1.0f;
	//}
	//UpdateProjection();
}

glm::vec3 Graphics::TwoDCamera::CalculatePosition() const
{
	return m_FocalPoint - GetForwardDirection() * m_Distance;
}

std::pair<float, float> Graphics::TwoDCamera::PanSpeed() const
{
	float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
	float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

	float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
	float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

	return { xFactor, yFactor };
}

float Graphics::TwoDCamera::RotationSpeed() const
{
	return 0.8f;
}

float Graphics::TwoDCamera::ZoomSpeed() const
{
	float distance = m_Distance * 0.2f;
	distance = std::max(distance, 0.0f);
	float speed = distance * distance;
	speed = std::min(speed, 100.0f); // max speed = 100
	return speed;
}