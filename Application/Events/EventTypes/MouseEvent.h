#pragma once

#include "Events/Event.h"
#include "Events/Codes/MouseCodes.h"
#include <chrono>
#include <future>

namespace Application {

	// What physically produced a pointer event. Lets consumers (ImGui source
	// tagging, tools, palm rejection) distinguish a finger from a Pencil from
	// a mouse/trackpad pointer without inspecting native events.
	enum class PointerType
	{
		Mouse = 0,
		Touch,
		Pencil,
		IndirectPointer
	};

	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(const float x, const float y, void* nativeEvent = nullptr,
			PointerType pointerType = PointerType::Mouse, float pressure = 1.0f)
			: m_MouseX(x), m_MouseY(y), Event(nativeEvent), m_PointerType(pointerType), m_Pressure(pressure) {}

		float GetX() const { return m_MouseX; }
		float GetY() const { return m_MouseY; }
		PointerType GetPointerType() const { return m_PointerType; }
		// Normalized 0..1 for Pencil/pressure-capable devices; 1.0 otherwise.
		float GetPressure() const { return m_Pressure; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
			EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	private:
		float m_MouseX, m_MouseY;
		PointerType m_PointerType;
		float m_Pressure;
	};

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(const float xOffset, const float yOffset,
			bool precise = false, bool momentum = false)
			: m_XOffset(xOffset), m_YOffset(yOffset), m_Precise(precise), m_Momentum(momentum) {}

		float GetXOffset() const { return m_XOffset; }
		float GetYOffset() const { return m_YOffset; }
		// True for trackpad/continuous scrolling (pixel deltas); false for a
		// clicky mouse wheel (line deltas).
		bool IsPrecise() const { return m_Precise; }
		// True while the OS is delivering inertial (momentum) scroll events.
		bool IsMomentum() const { return m_Momentum; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
			EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	private:
		float m_XOffset, m_YOffset;
		bool m_Precise;
		bool m_Momentum;
	};

	class MouseButtonEvent : public Event
	{
	public:
		MouseCode GetMouseButton() const { return m_Button; }
		PointerType GetPointerType() const { return m_PointerType; }
		float GetPressure() const { return m_Pressure; }

		EVENT_CLASS_TYPE(MouseButtonPressedOrReleased)
			EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)
	protected:
		MouseButtonEvent(const MouseCode button, void* nativeEvent,
			PointerType pointerType = PointerType::Mouse, float pressure = 1.0f)
			: m_Button(button), Event(nativeEvent), m_PointerType(pointerType), m_Pressure(pressure) {}

		MouseCode m_Button;
		PointerType m_PointerType;
		float m_Pressure;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(const MouseCode button, void* nativeEvent = nullptr,
			PointerType pointerType = PointerType::Mouse, float pressure = 1.0f)
			: MouseButtonEvent(button, nativeEvent, pointerType, pressure) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
			EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	private:
		std::chrono::milliseconds m_PressDuration;

	public:


		MouseButtonReleasedEvent(const MouseCode button, std::chrono::milliseconds _pressDuration = std::chrono::milliseconds(0), void* nativeEvent = nullptr,
			PointerType pointerType = PointerType::Mouse)
			: MouseButtonEvent(button, nativeEvent, pointerType), m_PressDuration(_pressDuration) {}
        
		std::chrono::milliseconds GetPressDuration() const { return m_PressDuration; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button << " pressed for: " << m_PressDuration;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
			EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	};

}