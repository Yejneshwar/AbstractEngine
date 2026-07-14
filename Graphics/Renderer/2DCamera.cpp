#include "2DCamera.h"
#include <iostream>
#include <Events/Input.h>
#include <algorithm>
#include <array>
#include <cmath>
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

Graphics::TwoDCamera::TwoDCamera(float nearClip, float farClip)
	: m_NearClip(nearClip), m_FarClip(farClip), Camera(glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, nearClip, farClip))
{
	// Establish valid world bounds + grid spacing immediately (the default
	// viewport size is refined by SetViewportSize on the first frame).
	UpdateProjection();
	UpdateView();
}

void Graphics::TwoDCamera::OnEvent(Application::Event& event)
{
	Application::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Application::MouseScrolledEvent>(APP_BIND_EVENT_FN(Graphics::TwoDCamera::OnMouseScroll));
	dispatcher.Dispatch<Application::MouseMovedEvent>(APP_BIND_EVENT_FN(Graphics::TwoDCamera::OnMouseMove));
    dispatcher.Dispatch<Application::MouseButtonPressedEvent>(APP_BIND_EVENT_FN(Graphics::TwoDCamera::OnMousePressed));
	dispatcher.Dispatch<Application::PinchGestureEvent>(APP_BIND_EVENT_FN(Graphics::TwoDCamera::OnPinch));
	dispatcher.Dispatch<Application::PanGestureEvent>(APP_BIND_EVENT_FN(Graphics::TwoDCamera::OnPanGesture));
}

// Pinch = zoom. The pinch scale maps to the ortho extents exactly (spread
// fingers 2x -> content 2x larger), so the content tracks the fingers.
// 2D zoom lives in the ortho projection, so mirror OnMouseScroll exactly:
// projection, view, and grid spacing must all be recomputed.
bool Graphics::TwoDCamera::OnPinch(Application::PinchGestureEvent& e)
{
	if (e.GetScaleDelta() > 0.0f)
		ZoomByFactor(1.0 / (double)e.GetScaleDelta());
	UpdateProjection();
	UpdateView();
	return false;
}

// Two-finger drag = pan the canvas; deltas are already view points, the same
// units MousePan expects from mouse-move panning.
bool Graphics::TwoDCamera::OnPanGesture(Application::PanGestureEvent& e)
{
	MousePan(glm::vec2(e.GetDeltaX(), e.GetDeltaY()));
	UpdateProjection();
	UpdateView();
	return false;
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
	// The 2D camera always looks straight down -Z (the old formula
	// normalized position - (-position), which was meaningless).
	return GetForwardDirection();
}

void Graphics::TwoDCamera::UpdateProjection()
{
	const double aspectRatio = (double)m_ViewportWidth / (double)m_ViewportHeight;
	m_AspectRatio = (float)aspectRatio;

	// Camera-relative ortho extents (the view matrix supplies the pan).
	const double halfWidth = m_zoom;
	const double halfHeight = m_zoom / aspectRatio;

	// True world-space bounds of the visible area, in double so labels and
	// mouse-world mapping stay exact at deep zoom. These INCLUDE the pan —
	// consumers must not add the focal point on top.
	this->worldXmin = (double)m_FocalPoint.x - halfWidth;
	this->worldXmax = (double)m_FocalPoint.x + halfWidth;
	this->worldYmin = (double)m_FocalPoint.y - halfHeight;
	this->worldYmax = (double)m_FocalPoint.y + halfHeight;

#if BUILDING_METAL
	// Metal clips NDC z to [0,1]; the GL-convention matrix maps depth to
	// [-1,1], silently clipping the NEAR HALF of the ortho volume (anything
	// above world z ~ +1 vanished). Use the zero-to-one variant.
	m_Projection = glm::orthoZO((float)-halfWidth, (float)halfWidth, (float)-halfHeight, (float)halfHeight, m_NearClip, m_FarClip);
	// Flip Y-axis to match coordinate system
	m_Projection[1][1] *= -1.0f;
#else
	m_Projection = glm::ortho((float)-halfWidth, (float)halfWidth, (float)-halfHeight, (float)halfHeight, m_NearClip, m_FarClip);
#endif

	// Grid spacing follows the visible extent; computing it here keeps it
	// valid from the first frame and for every zoom path.
	GetSpacing(m_ViewportWidth, worldXmin, worldXmax, gridMajorSpacing, gridMinorSpacing);
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
	// Trackpad two-finger scroll (precise, pixel deltas — includes the OS
	// momentum tail): pan the canvas, like the touch pan gesture. Zooming
	// stays on pinch / mouse wheel / Command|Ctrl+scroll.
	if (e.IsPrecise()
		&& !Application::Input::IsModifierDown(Application::Modifier::Command)
		&& !Application::Input::IsModifierDown(Application::Modifier::Control)) {
		// MousePan expects screen-pixel deltas; scrollingDelta already is
		// (and with natural scrolling it follows the fingers, matching the
		// iOS two-finger pan mapping).
		MousePan(glm::vec2(e.GetXOffset(), e.GetYOffset()));
		UpdateProjection();
		UpdateView();
		return false;
	}

	float delta = e.GetYOffset() * 0.1f;
	MouseZoom(delta);
	UpdateProjection();
	UpdateView();
	return false;
}

bool Graphics::TwoDCamera::OnMouseMove(Application::MouseMovedEvent& e)
{
	bool mLeft = Application::Input::IsMouseButtonPressed(Application::Mouse::ButtonLeft);
	bool mRight = Application::Input::IsMouseButtonPressed(Application::Mouse::ButtonRight);
	const glm::vec2& mouse{ Application::Input::GetMouseX(), Application::Input::GetMouseY() };
	glm::vec2 delta = (mouse - m_InitialMousePosition);
	m_InitialMousePosition = mouse;

	if (!mLeft && !mRight) return false;

	// Tap-vs-drag hysteresis: ignore movement until it exceeds the slop
	// radius from the press anchor (see OnMousePressed).
	if (!m_DragLatched) {
		constexpr float kDragThresholdPoints = 4.0f;
		if (glm::length(mouse - m_PressAnchor) < kDragThresholdPoints)
			return false;
		m_DragLatched = true;
	}

	if (mLeft)
		MousePan(delta);

	UpdateProjection();
	UpdateView();
	return false;
}

bool Graphics::TwoDCamera::OnMousePressed(Application::MouseButtonPressedEvent& e) {
    const glm::vec2& mouse{ Application::Input::GetMouseX(), Application::Input::GetMouseY() };
    m_InitialMousePosition = mouse;
    // Arm the tap-vs-drag threshold (a Pencil tap micro-jitters; it must
    // select without panning the canvas).
    m_PressAnchor = mouse;
    m_DragLatched = false;
    return false;
}

void Graphics::TwoDCamera::MousePan(const glm::vec2& delta)
{
	//Note: View will always lag behind the mouse due to delta being used. The pan has to "wait" for a change.
	glm::vec2 scale = {(this->worldXmax - this->worldXmin) / this->m_ViewportWidth, (this->worldYmax - this->worldYmin)/ this->m_ViewportHeight };
	auto C = delta * scale;
	m_FocalPoint += glm::vec3(-C.x, C.y, 0.0);
}

void Graphics::TwoDCamera::MouseRotate(const glm::vec2& delta)
{
	float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
	m_Yaw += yawSign * delta.x * RotationSpeed();
	m_Pitch += delta.y * RotationSpeed();
}

// Multiplicative zoom: strictly positive, scale-invariant steps, so zoom is
// "infinite" both ways within double range. (The old linear step with the
// one-way m_zoomLevel ratchet made zoom-out crawl after a deep zoom-in and
// could push m_zoom negative — a flipped ortho projection.)
void Graphics::TwoDCamera::ZoomByFactor(double factor)
{
	m_zoom = std::clamp(m_zoom * factor, 1e-9, 1e12);
}

void Graphics::TwoDCamera::MouseZoom(float delta)
{
	ZoomByFactor(std::exp((double)-delta));
}

glm::vec3 Graphics::TwoDCamera::CalculatePosition() const
{
	return m_FocalPoint - GetForwardDirection() * m_Distance;
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
