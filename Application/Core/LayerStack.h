#pragma once

#include "Layer.h"
#include <vector>

namespace Application {
	class LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(GUI::Layer* layer);
		void PushOverlay(GUI::Layer* overlay);
		void PopLayer(GUI::Layer* layer);
		void PopOverlay(GUI::Layer* overlay);

		std::vector<GUI::Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<GUI::Layer*>::iterator end() { return m_Layers.end(); }
		std::vector<GUI::Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<GUI::Layer*>::reverse_iterator rend() { return m_Layers.rend(); }

		std::vector<GUI::Layer*>::const_iterator begin() const { return m_Layers.begin(); }
		std::vector<GUI::Layer*>::const_iterator end()	const { return m_Layers.end(); }
		std::vector<GUI::Layer*>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
		std::vector<GUI::Layer*>::const_reverse_iterator rend() const { return m_Layers.rend(); }
	private:
		std::vector<GUI::Layer*> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};
}