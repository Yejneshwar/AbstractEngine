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

	void Renderer::Clear(float alpha) {
		RenderCommand::Clear(alpha);
	}

	void Renderer::DepthTest(bool enable)
	{
		RenderCommand::DepthTest(enable);
	}
	
	void Renderer::PolygonSmooth(bool enable)
	{
		RenderCommand::PolygonSmooth(enable);
	}

	void Renderer::ClearStencil() {
		RenderCommand::ClearStencil();
	}

	void Renderer::ClearBuffers() {
		RenderCommand::ClearBuffers();
	}

	void Renderer::EnableStencil() {
		RenderCommand::EnableStencil();
	}

	void Renderer::DisableStencil() {
		RenderCommand::DisableStencil();
	}

	void Renderer::SetStencilFunc(unsigned int func, bool ref, uint8_t mask) {
		RenderCommand::SetStencilFunc(func , ref, mask);
	}

	void Renderer::SetStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass) {
		RenderCommand::SetStencilOp(sfail, dpfail, dppass);
	}
}
