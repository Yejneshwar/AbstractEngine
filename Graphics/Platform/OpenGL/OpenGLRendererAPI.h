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
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;

		virtual void DrawGridTriangles() override;
		
		virtual void SetLineWidth(float width) override;
		virtual void SetRendererMode(int mode) override;
		virtual void SetRendererModeToDefault() override;
		virtual void DepthTest(bool state) override;

		virtual void InitOtiBuffers(int width, int height) override;

		virtual void BindOtiBuffers() override;
		virtual void UnBindOtiBuffers() override;
		virtual void ClearOtiBuffers() override;

		virtual uint32_t GetOitColorBuffer(int index) override;
	private:
		unsigned int m_imgAbufferImage;
		unsigned int m_imgAuxImage;
		unsigned int m_imgAbufferBuffer;
		int m_width;
		int m_height;
		// Allocate temporary storage for clearing the image
		std::vector<unsigned int> m_clearData;
	};


}
