#include "GraphicsCore.h"
#include "Renderer/RendererAPI.h"

#ifdef BUILDING_METAL
#include "Platform/Metal/MetalRendererAPI.h"
#else
#include "Platform/OpenGL/OpenGLRendererAPI.h"
#endif

namespace Graphics {
#if __APPLE__
    RendererAPI::API RendererAPI::s_API = RendererAPI::API::Metal;
#else
	RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;
#endif

	Scope<RendererAPI> RendererAPI::Create()
	{
#if __APPLE__
        return CreateScope<MetalRendererAPI>();
#else
		switch (s_API)
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLRendererAPI>();
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}
}
