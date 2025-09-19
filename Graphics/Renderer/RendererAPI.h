#pragma once

#include "Renderer/VertexArray.h"

#include <glm/glm.hpp>

namespace Graphics {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1, Metal = 2
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear(float alpha = 1.0) = 0;
		virtual void ClearStencil() = 0;
		virtual void DepthTest(bool enable) = 0;
		virtual void PolygonSmooth(bool enable) = 0;
		virtual void ClearBuffers() = 0;
        
        virtual void BeginLoop() = 0;
        virtual void EndLoop() = 0;

		virtual void EnableStencil() = 0;
		virtual void DisableStencil() = 0;
		virtual void SetStencilFunc(unsigned int func, bool ref, uint8_t mask) = 0;
		virtual void SetStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass) = 0;

		virtual void DrawNonIndexed(const Ref<VertexArray>& vertexArray, uint32_t count = 0, uint32_t start = 0) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) = 0;
		virtual void DrawLinesIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount) = 0;
		virtual void DrawLinesInstancedBaseInstance(const Ref<VertexArray>& vertexArray, uint32_t filrst, uint32_t vertexCount, uint32_t instanceCount, uint32_t baseInstance) = 0;
		virtual void DrawWireFrameCube(const std::vector<glm::dvec3>& cube, const float& thickness) = 0;
		virtual void DrawGridTriangles() = 0;
		
		virtual void SetLineWidth(float width) = 0;

		virtual void SetRendererMode(int mode) = 0;
		virtual void SetRendererModeToDefault() = 0;

		static API GetAPI() { return s_API; }
		static Scope<RendererAPI> Create();
	private:
		static API s_API;
	};

}
