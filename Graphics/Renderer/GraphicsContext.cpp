#include "GraphicsCore.h"
#include "Renderer/GraphicsContext.h"

#include "Renderer/Renderer.h"

#if BUILDING_METAL
#include "Platform/Metal/MetalContext.h"
#else
#include "Platform/OpenGL/OpenGLContext.h"
#endif

namespace Graphics {

	Scope<GraphicsContext> GraphicsContext::Create(void* window)
	{
#if BUILDING_METAL
        return CreateScope<MetalContext>(window);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

}
