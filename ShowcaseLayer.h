#pragma once

// PBR showcase scene for the engine TestGUI: a metallic x roughness sphere
// grid, a torus knot, a ground plane, and flat/emissive examples — all
// retained meshes with live-editable materials and working picking.

#include <Core/Layer.h>
#include <Renderer/BatchRenderer.h>

#include <string>
#include <vector>

namespace GUI {

	class ShowcaseLayer : public Layer {
	public:
		ShowcaseLayer() : Layer("ShowcaseLayer") {}

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdateLayer() override {}
		void OnDrawUpdate() override;
		void OnImGuiRender() override;
		void OnEvent(Application::Event& event) override {}
		void OnSelection(int objectId, bool state) override;

	private:
		struct ShowcaseObject {
			std::string name;
			int pickId = -1;
			Graphics::BatchRenderer::MeshHandle mesh = 0;
			Graphics::BatchRenderer::MaterialHandle material = 0;
			bool editable = true;
		};

		void AddMesh(const std::string& name, const std::vector<double>& vertices, const std::vector<uint32_t>& indices,
		             const GUI::DataType::vec4& color, const Graphics::MaterialDesc& materialDesc, bool editable = true);

		std::vector<ShowcaseObject> m_Objects;
		int m_SelectedPickId = -1;
		int m_NextPickId = 1000;
		bool m_Enabled = true;
	};

}
