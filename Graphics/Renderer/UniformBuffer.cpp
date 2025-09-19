#include "GraphicsCore.h"
#include "UniformBuffer.h"

#include "Renderer/Renderer.h"

#ifdef BUILDING_METAL
#include "Platform/Metal/MetalUniformBuffer.h"
#else
#include "Platform/OpenGL/OpenGLUniformBuffer.h"
#endif

namespace Graphics {

	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding, std::string label)
	{
#ifdef BUILDING_METAL
        return CreateRef<MetalUniformBuffer>(size, binding, label);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLUniformBuffer>(size, binding);
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

}
