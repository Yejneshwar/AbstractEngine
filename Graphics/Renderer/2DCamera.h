#pragma once 

#include "Camera.h"
#include "glm/gtx/quaternion.hpp"
#include "Events/Event.h"
#include <Events/EventTypes/MouseEvent.h>

namespace Graphics {

	class TwoDCamera : public Camera
	{
	public:
		TwoDCamera() = default;
		TwoDCamera(float nearClip, float farClip);

		//void OnUpdate(double x, double y, int leftButton, int rightButton);
		void OnUpdate();
		void OnEvent(Application::Event& event);

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		inline void SetViewportSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; UpdateProjection(); UpdateView(); }

		glm::mat4 GetViewMatrix() const { return m_ViewMatrix; }
		glm::mat4 GetViewProjection() const { return m_Projection * m_ViewMatrix; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		const glm::vec3& GetPosition() const { return m_Position; }
		glm::quat GetOrientation() const;
		glm::vec3 GetViewDirection() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }
		//bool OnMouseScroll(double offset);

		void SetMousePos(glm::vec2 mousePos) { m_InitialMousePosition = mousePos; }

		void ResetFocalPoint() { m_FocalPoint = { 0.0f, 0.0f, 0.0f }; UpdateView();}

		void SetPosition(const glm::vec3& position) { m_Position = position; UpdateView(); }

		glm::vec3 GetFocalPoint() { return m_FocalPoint; }
		void SetFocalPoint(glm::vec3 point) { m_FocalPoint = point; UpdateView(); }

		double getWorldXmin() const { return worldXmin; }
		double getWorldXmax() const { return worldXmax; }
		double getWorldYmin() const { return worldYmin; }
		double getWorldYmax() const { return worldYmax; }
		double getAspectRatio() const { return m_AspectRatio; }
		double getGridMinorSpacing() const { return gridMinorSpacing; }
		double getGridMajorSpacing() const { return gridMajorSpacing; }
		double getZoom() const { return m_zoom;  }
	private:
		void UpdateProjection();
		void UpdateView();


		bool OnMouseScroll(Application::MouseScrolledEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		float RotationSpeed() const;
		float ZoomSpeed() const;
	private:
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };

		float m_Distance = 1.0f;
		double m_zoom = 4.0;
		double m_zoomLevel = 1.0;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1280, m_ViewportHeight = 720;

		double worldXmin = 0, worldXmax = 0, worldYmin = 0, worldYmax = 0;
		float gridMinorSpacing = 0.0f, gridMajorSpacing = 0.0f;
	};

}