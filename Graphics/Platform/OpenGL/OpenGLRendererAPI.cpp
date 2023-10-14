#include "GraphicsCore.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

#include <glad/gl.h>

#include "Logger.h"

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
		glEnable(GL_LINE_SMOOTH);
		//glEnable(GL_POLYGON_SMOOTH); // This sorta turns everything into a wireframe?
		glEnable(GL_MULTISAMPLE);
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
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::ClearBuffers()
	{
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
			LOG_FATAL_STREAM << "Index buffer not bound to vertexArray";
			return;
		}
		vertexArray->Bind();
		if (indexCount < 0) indexCount = indexBuffer->GetCount();
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void OpenGLRendererAPI::DrawLinesIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
		const auto& indexBuffer = vertexArray->GetIndexBuffer();
		if (indexBuffer == nullptr) {
			LOG_FATAL_STREAM << "Index buffer not bound to vertexArray";
			return;
		}
		vertexArray->Bind();
		glDrawElements(GL_LINES, indexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawWireFrameCube(const std::vector<glm::dvec3>& cube, const float& thickness) {
		glLineWidth(thickness);
		glColor3f(1.0,1.0,1.0);
		glBegin(GL_LINES);
		glVertex3d(0, 0, 0);
		glVertex3d(-0.3, 0.5, 0.5);
		std::cout << cube.at(0).x << std::endl;
		glVertex3d(cube.at(0).x, cube.at(0).y, cube.at(0).z); glVertex3d(cube.at(1).x, cube.at(1).y, cube.at(1).z);
		glVertex3d(cube.at(0).x, cube.at(0).y, cube.at(0).z); glVertex3d(cube.at(3).x, cube.at(3).y, cube.at(3).z);
		glVertex3d(cube.at(0).x, cube.at(0).y, cube.at(0).z); glVertex3d(cube.at(4).x, cube.at(4).y, cube.at(4).z);
		glVertex3d(cube.at(1).x, cube.at(1).y, cube.at(1).z); glVertex3d(cube.at(2).x, cube.at(2).y, cube.at(2).z);
		glVertex3d(cube.at(1).x, cube.at(1).y, cube.at(1).z); glVertex3d(cube.at(5).x, cube.at(5).y, cube.at(5).z);
		glVertex3d(cube.at(2).x, cube.at(2).y, cube.at(2).z); glVertex3d(cube.at(3).x, cube.at(3).y, cube.at(3).z);
		glVertex3d(cube.at(2).x, cube.at(2).y, cube.at(2).z); glVertex3d(cube.at(6).x, cube.at(6).y, cube.at(6).z);
		glVertex3d(cube.at(3).x, cube.at(3).y, cube.at(3).z); glVertex3d(cube.at(7).x, cube.at(7).y, cube.at(7).z);
		glVertex3d(cube.at(4).x, cube.at(4).y, cube.at(4).z); glVertex3d(cube.at(5).x, cube.at(5).y, cube.at(5).z);
		glVertex3d(cube.at(4).x, cube.at(4).y, cube.at(4).z); glVertex3d(cube.at(7).x, cube.at(7).y, cube.at(7).z);
		glVertex3d(cube.at(5).x, cube.at(5).y, cube.at(5).z); glVertex3d(cube.at(6).x, cube.at(6).y, cube.at(6).z);
		glVertex3d(cube.at(6).x, cube.at(6).y, cube.at(6).z); glVertex3d(cube.at(7).x, cube.at(7).y, cube.at(7).z);
		glEnd();
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

}
