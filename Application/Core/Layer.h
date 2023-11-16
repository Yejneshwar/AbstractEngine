#pragma once


#include <string>
#include "Events/Event.h"

namespace GUI {
	class Layer
	{
	public:
		Layer(const std::string& name = "Layer") : m_DebugName(name) {}
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdateLayer() {}
		virtual void OnDrawUpdate() {}
		virtual void OnEvent(Application::Event& event) {}
		virtual void OnImGuiRender() {}

		inline void UpdateLayer(bool update = true) { m_updateLayers = update; }

		bool IsUpdateLayer() const { return m_updateLayers; }

		const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
		bool m_updateLayers = false;
	};
}