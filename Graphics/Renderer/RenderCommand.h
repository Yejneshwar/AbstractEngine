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

		static void Clear(float alpha = 1.0)
		{
			s_RendererAPI->Clear(alpha);
		}

		static void ClearStencil()
		{
			s_RendererAPI->ClearStencil();
		}

		static void DepthTest(bool enable)
		{
			s_RendererAPI->DepthTest(enable);
		}
		
		static void PolygonSmooth(bool enable)
		{
			s_RendererAPI->PolygonSmooth(enable);
		}

		static void ClearBuffers()
		{
			s_RendererAPI->ClearBuffers();
		}
        
        static void BeginLoop()
        {
            s_RendererAPI->BeginLoop();
        }
        
        static void EndLoop()
        {
            s_RendererAPI->EndLoop();
        }


		static void EnableStencil(){
			s_RendererAPI->EnableStencil();
		};

		static void DisableStencil(){
			s_RendererAPI->DisableStencil();
		};

		static void SetStencilFunc(unsigned int func, bool ref, uint8_t mask){
			s_RendererAPI->SetStencilFunc(func, ref, mask);
		};

		static void SetStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass){
			s_RendererAPI->SetStencilOp(sfail, dpfail, dppass);
		};

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

		static void DrawLinesIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawLinesIndexed(vertexArray, indexCount);
		}

		static void DrawLinesInstancedBaseInstance(const Ref<VertexArray>& vertexArray, uint32_t filrst, uint32_t vertexCount, uint32_t instanceCount, uint32_t baseInstance)
		{
			s_RendererAPI->DrawLinesInstancedBaseInstance(vertexArray, filrst, vertexCount, instanceCount, baseInstance);
		}

		static void DrawWireFrameCube(const std::vector<glm::dvec3>& cube, const float& thickness) {
			s_RendererAPI->DrawWireFrameCube(cube, thickness);
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
