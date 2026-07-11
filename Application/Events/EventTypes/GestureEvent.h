#pragma once

#include "Events/Event.h"

namespace Application {

	// Continuous multi-touch / trackpad gesture phases, mirroring the phases
	// of NSGestureRecognizer / UIGestureRecognizer.
	enum class GesturePhase
	{
		Began = 0,
		Changed,
		Ended,
		Cancelled
	};

	class GestureEvent : public Event
	{
	public:
		GesturePhase GetPhase() const { return m_Phase; }
		// Gesture centroid in view coordinates (same space as MouseMovedEvent).
		float GetCenterX() const { return m_CenterX; }
		float GetCenterY() const { return m_CenterY; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	protected:
		GestureEvent(GesturePhase phase, float centerX, float centerY, void* nativeEvent)
			: Event(nativeEvent), m_Phase(phase), m_CenterX(centerX), m_CenterY(centerY) {}

		GesturePhase m_Phase;
		float m_CenterX, m_CenterY;
	};

	// Pinch-to-zoom. GetScaleDelta() is the multiplicative scale change since
	// the PREVIOUS event of the gesture (1.0 = no change), so consumers can
	// apply it incrementally without tracking gesture state.
	class PinchGestureEvent : public GestureEvent
	{
	public:
		PinchGestureEvent(float scaleDelta, GesturePhase phase, float centerX = 0.0f, float centerY = 0.0f, void* nativeEvent = nullptr)
			: GestureEvent(phase, centerX, centerY, nativeEvent), m_ScaleDelta(scaleDelta) {}

		float GetScaleDelta() const { return m_ScaleDelta; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "PinchGestureEvent: " << m_ScaleDelta;
			return ss.str();
		}

		EVENT_CLASS_TYPE(GesturePinch)
	private:
		float m_ScaleDelta;
	};

	// Two-finger rotation. GetAngleDelta() is radians since the previous
	// event of the gesture; positive = counter-clockwise.
	class RotateGestureEvent : public GestureEvent
	{
	public:
		RotateGestureEvent(float angleDelta, GesturePhase phase, float centerX = 0.0f, float centerY = 0.0f, void* nativeEvent = nullptr)
			: GestureEvent(phase, centerX, centerY, nativeEvent), m_AngleDelta(angleDelta) {}

		float GetAngleDelta() const { return m_AngleDelta; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "RotateGestureEvent: " << m_AngleDelta;
			return ss.str();
		}

		EVENT_CLASS_TYPE(GestureRotate)
	private:
		float m_AngleDelta;
	};

	// Two-finger (touch) pan. Deltas are view points since the previous event.
	class PanGestureEvent : public GestureEvent
	{
	public:
		PanGestureEvent(float deltaX, float deltaY, GesturePhase phase, float centerX = 0.0f, float centerY = 0.0f, void* nativeEvent = nullptr)
			: GestureEvent(phase, centerX, centerY, nativeEvent), m_DeltaX(deltaX), m_DeltaY(deltaY) {}

		float GetDeltaX() const { return m_DeltaX; }
		float GetDeltaY() const { return m_DeltaY; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "PanGestureEvent: " << m_DeltaX << ", " << m_DeltaY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(GesturePan)
	private:
		float m_DeltaX, m_DeltaY;
	};

}
