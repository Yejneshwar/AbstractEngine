#include "Platform/OpenGL/OpenGLContext.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <GraphicsCore.h>

namespace Graphics {

	struct {
		int major = 4, minor = 5, rev;
	} GLVersion;

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

		//glfwGetVersion(&GLVersion.major, &GLVersion.minor, &GLVersion.rev);

		GRAPHICS_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), std::format("AbstractEngine requires at least OpenGL version 4.5! \n OpenGL {}.{}rev{}", GLVersion.major, GLVersion.minor, GLVersion.rev));
	}

	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

}
