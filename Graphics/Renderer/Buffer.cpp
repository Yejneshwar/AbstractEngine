#include "GraphicsCore.h"
#include "Renderer/Buffer.h"

#include "Renderer/Renderer.h"

#ifdef BUILDING_METAL
#include "Platform/Metal/MetalBuffer.h"
#else
#include "Platform/OpenGL/OpenGLBuffer.h"
#endif

namespace Graphics {

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size, std::string label)
	{
#ifdef BUILDING_METAL
        return CreateRef<MetalVertexBuffer>(size, label);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLVertexBuffer>(size);
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size, std::string label)
	{
#ifdef BUILDING_METAL
        return CreateRef<MetalVertexBuffer>(vertices, size, label);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLVertexBuffer>(vertices, size);
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t size, std::string label)
	{
#ifdef BUILDING_METAL
        return CreateRef<MetalIndexBuffer>(indices, size, label);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLIndexBuffer>(indices, size);
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t size, std::string label)
	{
#ifdef BUILDING_METAL
        return CreateRef<MetalIndexBuffer>(size, label);
#else
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLIndexBuffer>(size);
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}


}
