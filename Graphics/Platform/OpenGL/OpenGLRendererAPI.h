#pragma once

#include "Renderer/RendererAPI.h"

namespace Graphics {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;

		virtual void ClearBuffers() override;

		virtual void DrawNonIndexed(const Ref<VertexArray>& vertexArray, uint32_t count = 0, uint32_t start = 0) override;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = -1) override;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;
		virtual void DrawLinesIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount) override;


		virtual void DrawWireFrameCube(const std::vector<glm::dvec3>& cube, const float& thickness) override;

		virtual void DrawGridTriangles() override;
		
		virtual void SetLineWidth(float width) override;
		virtual void SetRendererMode(int mode) override;
		virtual void SetRendererModeToDefault() override;
	};


}
