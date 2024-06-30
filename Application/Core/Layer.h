#pragma once


#include <string>
#include "Events/Event.h"

namespace GUI {
	class Layer
	{
	public:
		Layer(const std::string& name = "Layer") : m_DebugName(name) {}
		virtual ~Layer() = default;

		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
		virtual void OnUpdateLayer() = 0;
		virtual void OnDrawUpdate() = 0;
		virtual void OnEvent(Application::Event& event) = 0;
		virtual void OnSelection(int objectId, bool state) = 0;
		virtual void OnImGuiRender() = 0;

		static inline void UpdateLayer(bool update = true) { m_updateLayers = update; }

		bool IsUpdateLayer() const { return m_updateLayers; }

		const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
		static bool m_updateLayers;
	};
}