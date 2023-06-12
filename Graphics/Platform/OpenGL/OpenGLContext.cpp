#include "Platform/OpenGL/OpenGLContext.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <GraphicsCore.h>

namespace Graphics {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		GRAPHICS_CORE_ASSERT(windowHandle, "Window handle is null!");
	}

	void OpenGLContext::Init()
	{
		

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
		GRAPHICS_CORE_ASSERT(status, "Failed to initialize Glad!");

		GRAPHICS_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Hazel requires at least OpenGL version 4.5!");
	}

	void OpenGLContext::SwapBuffers()
	{
		

		glfwSwapBuffers(m_WindowHandle);
	}

}
