#pragma once

#include "Renderer/RendererAPI.h"

namespace Graphics {

	class RenderCommand
	{
	public:
		static void Init()
		{
			s_RendererAPI->Init();
		}

		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		static void Clear()
		{
			s_RendererAPI->Clear();
		}

		static void DrawNonIndexed(const Ref<VertexArray>& vertexArray, uint32_t count = 0, uint32_t start = 0)
		{
			s_RendererAPI->DrawNonIndexed(vertexArray, count, start);
		}

		static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

		static void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
		{
			s_RendererAPI->DrawLines(vertexArray, vertexCount);
		}

		static void DrawGridTriangles() {
			s_RendererAPI->DrawGridTriangles();
		}

		static void SetLineWidth(float width)
		{
			s_RendererAPI->SetLineWidth(width);
		}

		static void SetRenderMode(int mode)
		{
			s_RendererAPI->SetRendererMode(mode);
		}

		static void SetRenderModeToDefault()
		{
			s_RendererAPI->SetRendererModeToDefault();
		}
	private:
		static Scope<RendererAPI> s_RendererAPI;
	};

}
