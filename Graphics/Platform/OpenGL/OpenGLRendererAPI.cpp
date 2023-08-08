#include "GraphicsCore.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

#include <glad/gl.h>

namespace Graphics {
	
	void OpenGLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		//switch (severity)
		//{
		//	case GL_DEBUG_SEVERITY_HIGH:         HZ_CORE_CRITICAL(message); return;
		//	case GL_DEBUG_SEVERITY_MEDIUM:       HZ_CORE_ERROR(message); return;
		//	case GL_DEBUG_SEVERITY_LOW:          HZ_CORE_WARN(message); return;
		//	case GL_DEBUG_SEVERITY_NOTIFICATION: HZ_CORE_TRACE(message); return;
		//}
		//
		//GRAPHICS_CORE_ASSERT(false, "Unknown severity level!");
	}

	void OpenGLRendererAPI::Init()
	{
		

	#ifdef HZ_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);
		
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
	#endif

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		//glDepthFunc(GL_LESS);

		glEnable(GL_LINE_SMOOTH);
	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::Clear()
	{
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::ClearBuffers()
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawNonIndexed(const Ref<VertexArray>& vertexArray, uint32_t count, uint32_t start)
	{
		vertexArray->Bind();
		glDrawArrays(GL_TRIANGLES, start, count);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
		const auto& indexBuffer = vertexArray->GetIndexBuffer();
		if (indexBuffer == nullptr) {
			std::cout << "Index buffer not bound to vertexArray" << std::endl;
			return;
		}
		vertexArray->Bind();
		glDrawElements(GL_TRIANGLES, indexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void OpenGLRendererAPI::DrawGridTriangles(){
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
	}

	void OpenGLRendererAPI::SetLineWidth(float width)
	{
		glLineWidth(width);
	}

	void OpenGLRendererAPI::SetRendererMode(GLint mode) 
	{
		glPolygonMode(GL_FRONT_AND_BACK, mode);
	}

	void OpenGLRendererAPI::SetRendererModeToDefault()
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	void OpenGLRendererAPI::DepthTest(bool state) {


		if (state) {
			//glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			glBlendFunci(1, GL_ONE, GL_ONE);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
			glBlendEquation(GL_FUNC_ADD);
			glDisable(GL_CULL_FACE);
			
		}
		else {
			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			//glDepthMask(GL_FALSE); // enable depth writes so glClear won't ignore clearing the depth buffer
		}
	}

	void OpenGLRendererAPI::InitOtiBuffers(int width, int height)
	{
		m_width = width;
		m_height = height;
		GLuint64 size = static_cast<GLuint64>(width) * static_cast<GLuint64>(height) * static_cast <GLuint64>(8) * static_cast <GLuint64>(sizeof(glm::uvec2));
		m_clearData = std::vector<unsigned int>(size);

		glGenBuffers(1, &m_imgAbufferBuffer);
		glBindBuffer(GL_TEXTURE_BUFFER, m_imgAbufferBuffer);
		glBufferData(GL_TEXTURE_BUFFER, size, nullptr, GL_DYNAMIC_COPY);

		// Generate image object for imgAbuffer
		glGenTextures(1, &m_imgAbufferImage);
		glBindTexture(GL_TEXTURE_BUFFER, m_imgAbufferImage);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Allocate storage for imgAbuffer
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32UI, m_imgAbufferBuffer);
		// Make sure imgAbufferBuffer is created and populated correctly before this point

		// Generate image object for imgAux
		glGenTextures(1, &m_imgAuxImage);
		glBindTexture(GL_TEXTURE_2D, m_imgAuxImage);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Allocate storage for imgAux
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, width, height);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);


	}

	void OpenGLRendererAPI::BindOtiBuffers()
	{
		glBindBuffer(GL_TEXTURE_BUFFER, m_imgAbufferBuffer);
		glBindImageTexture(1, m_imgAbufferImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32UI);
		glBindImageTexture(2, m_imgAuxImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	}

	void OpenGLRendererAPI::UnBindOtiBuffers()
	{
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32UI);
		glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	}

	void OpenGLRendererAPI::ClearOtiBuffers()
	{
		// Bind the image texture
		glBindTexture(GL_TEXTURE_2D, m_imgAuxImage);

		

		// Upload the clear data to the image texture
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, GL_RED_INTEGER, GL_UNSIGNED_INT, m_clearData.data());

		// Unbind the image texture
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	uint32_t OpenGLRendererAPI::GetOitColorBuffer(int index)
	{
		if (index == 0) {
			return m_imgAbufferImage;
		}

		else return m_imgAuxImage;
	}

}
