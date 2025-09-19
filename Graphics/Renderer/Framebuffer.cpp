#include "Renderer/Framebuffer.h"

#include "Renderer/Renderer.h"

#ifdef BUILDING_METAL
#include "Platform/Metal/MetalFrameBuffer.h"
#else
#include "Platform/OpenGL/OpenGLFrameBuffer.h"
#endif

namespace Graphics {
	
	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
#ifdef BUILDING_METAL
        return CreateRef<MetalFramebuffer>(spec);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLFramebuffer>(spec);
		}
#endif
		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
