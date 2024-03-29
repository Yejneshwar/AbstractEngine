#pragma once

#include "Renderer/RenderCommand.h"

#include "Renderer/OrthographicCamera.h"
#include "Renderer/Shader.h"

namespace Graphics {

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();
		
		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(OrthographicCamera& camera);
		static void EndScene();

		static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

		static void DrawGridTriangles();

		static void Clear(float alpha = 1.0);
		static void DepthTest(bool enable);
		
		static void PolygonSmooth(bool enable);

		static void ClearStencil();

		static void ClearBuffers();

		static void EnableStencil();
		static void DisableStencil();
		static void SetStencilFunc(unsigned int func, bool ref, uint8_t mask);
		static void SetStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass);

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static Scope<SceneData> s_SceneData;
	};
}
