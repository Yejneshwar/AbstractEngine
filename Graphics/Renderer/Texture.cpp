
#include "Texture.h"
#include "Renderer/Renderer.h"

#if BUILDING_METAL
#include "Platform/Metal/MetalTexture.h"
#else
#include "Platform/OpenGL/OpenGLTexture.h"
#endif

namespace Graphics {

	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height, TextureFormat format, uint32_t mipLevels)
	{
#if BUILDING_METAL
        return CreateRef<MetalTexture2D>(width, height, format, mipLevels);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(width, height, format, mipLevels);
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
#if BUILDING_METAL
        return CreateRef<MetalTexture2D>(path);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(path);
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

}
