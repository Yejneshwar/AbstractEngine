#include "LayerStack.h"


Application::LayerStack::~LayerStack()
{
	for (GUI::Layer* layer : m_Layers)
	{
		layer->OnDetach();
		delete layer;
	}
}

void Application::LayerStack::PushLayer(GUI::Layer* layer)
{
	m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
	m_LayerInsertIndex++;
}

void Application::LayerStack::PushOverlay(GUI::Layer* overlay)
{
	m_Layers.emplace_back(overlay);
}

void Application::LayerStack::PopLayer(GUI::Layer* layer)
{
	auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
	if (it != m_Layers.begin() + m_LayerInsertIndex)
	{
		layer->OnDetach();
		m_Layers.erase(it);
		m_LayerInsertIndex--;
	}
}

void Application::LayerStack::PopOverlay(GUI::Layer* overlay)
{
	auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
	if (it != m_Layers.end())
	{
		overlay->OnDetach();
		m_Layers.erase(it);
	}
}