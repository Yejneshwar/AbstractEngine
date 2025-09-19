#include "GraphicsCore.h"
#include "Renderer/GraphicsContext.h"

#include "Renderer/Renderer.h"

#ifdef BUILDING_METAL
#include "Platform/Metal/MetalContext.h"
#else
#include "Platform/OpenGL/OpenGLContext.h"
#endif

namespace Graphics {

	Ref<GraphicsContext> GraphicsContext::Create(void* window)
	{
#ifdef BUILDING_METAL
        return CreateRef<MetalContext>(window);
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
