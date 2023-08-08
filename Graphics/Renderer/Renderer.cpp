#include "Renderer/Renderer.h"
#include "Renderer/Renderer2D.h"

namespace Graphics {

	Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();

	void Renderer::Init()
	{
		RenderCommand::Init();
		//Renderer2D::Init();
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(OrthographicCamera& camera)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
	{
		shader->Bind();
		shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		shader->SetMat4("u_Transform", transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}

	void Renderer::DrawGridTriangles()
	{
		RenderCommand::DrawGridTriangles();
	}

	void Renderer::Clear() {
		RenderCommand::Clear();
	}

	void Renderer::ClearBuffers() {
		RenderCommand::ClearBuffers();
	}

	void Renderer::DepthTest(bool state)
	{
		RenderCommand::DepthTest(state);
	}

	void Renderer::InitOtiBuffers(int width, int height)
	{
		RenderCommand::InitOtiBuffers(width, height);
	}

	void Renderer::BindOtiBuffers()
	{
		RenderCommand::BindOtiBuffers();
	}

	void Renderer::UnBindOtiBuffers()
	{
		RenderCommand::UnBindOtiBuffers();
	}

	void Renderer::ClearOtiBuffers()
	{
		RenderCommand::ClearOtiBuffers();
	}

	uint32_t Renderer::GetOitColorBuffer(int index)
	{
		return RenderCommand::GetOitColorBuffer(index);
	}
}
