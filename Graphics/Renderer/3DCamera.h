#pragma once 

#include "Camera.h"
#include "glm/gtx/quaternion.hpp"
#include "Events/Event.h"
#include <Events/EventTypes/MouseEvent.h>


namespace Graphics {

	class ThreeDCamera : public Camera
	{
	public:
		ThreeDCamera() = default;
		ThreeDCamera(float fov, float aspectRatio, float nearClip, float farClip);

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

		double getWorldXmin() const { return 0; }
		double getWorldXmax() const { return 0; }
		double getWorldYmin() const { return 0; }
		double getWorldYmax() const { return 0; }

		glm::vec4 getViewport() const { return glm::vec4(m_ViewportWidth, m_ViewportHeight, m_ViewportWidth * m_ViewportHeight, 0); }
		double getAspectRatio() const { return m_AspectRatio; }
		double getZoom() const { return 0; }

		double getGridMinorSpacing() const { return 0; }
		double getGridMajorSpacing() const { return 0; }


		//bool OnMouseScroll(double offset);

		void SetMousePos(glm::vec2 mousePos) { m_InitialMousePosition = mousePos; }

		void ResetFocalPoint() { m_FocalPoint = { 0.0f, 0.0f, 0.0f }; UpdateView(); }

		void SetPosition(const glm::vec3& position) { m_Position = position; UpdateView(); }

		glm::vec3 GetFocalPoint() { return m_FocalPoint; }
		void SetFocalPoint(glm::vec3 point) { m_FocalPoint = point; UpdateView(); }

	private:
		void UpdateProjection();
		void UpdateView();


		bool OnMouseScroll(Application::MouseScrolledEvent& e);
		bool OnMouseMove(Application::MouseMovedEvent& e);


		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;
	private:
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1280, m_ViewportHeight = 720;
	};

}