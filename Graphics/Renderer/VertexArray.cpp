#include "GraphicsCore.h"
#include "Renderer/VertexArray.h"

#include "Renderer/Renderer.h"

#ifdef BUILDING_METAL
#include "Platform/Metal/MetalVertexArray.h"
#else
#include "Platform/OpenGL/OpenGLVertexArray.h"
#endif


namespace Graphics {

	Ref<VertexArray> VertexArray::Create()
	{
#ifdef BUILDING_METAL
        return CreateRef<MetalVertexArray>();
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLVertexArray>();
            case RendererAPI::API::Metal:	GRAPHICS_CORE_ASSERT(false, "MetalAPI does not support Vertex Array"); return nullptr;
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

}
