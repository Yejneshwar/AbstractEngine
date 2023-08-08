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

		static void Clear();

		static void ClearBuffers();

		static void DepthTest(bool state);

		static void InitOtiBuffers(int width, int height);

		static void BindOtiBuffers();
		static void UnBindOtiBuffers();
		static void ClearOtiBuffers();

		static uint32_t GetOitColorBuffer(int index);

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static Scope<SceneData> s_SceneData;
	};
}
