#pragma once

#include <glm/glm.hpp>
#include "Events/Event.h"

namespace Graphics {

	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projection)
			: m_Projection(projection) {}

		virtual ~Camera() = default;

		virtual void OnUpdate() = 0;
		virtual void OnEvent(Application::Event& event) = 0;

		virtual inline void SetViewportSize(float width, float height) = 0;

		const glm::mat4& GetProjection() const { return m_Projection; }

		virtual double getWorldXmin() const = 0;
		virtual double getWorldXmax() const = 0;
		virtual double getWorldYmin() const = 0;
		virtual double getWorldYmax() const = 0;

		virtual double getAspectRatio() const = 0;
		virtual double getZoom() const = 0;

		virtual double getGridMinorSpacing() const = 0;
		virtual double getGridMajorSpacing() const = 0;

		virtual const glm::vec3& GetPosition() const = 0;
		virtual glm::vec3 GetViewDirection() const = 0;

		virtual void ResetFocalPoint() = 0;
		virtual glm::vec3 GetFocalPoint() = 0;
		virtual void SetFocalPoint(glm::vec3 point) = 0;

		virtual glm::mat4 GetViewMatrix() const = 0;
		virtual glm::mat4 GetViewProjection() const = 0;

	protected:
		glm::mat4 m_Projection = glm::mat4(1.0f);
	};

}