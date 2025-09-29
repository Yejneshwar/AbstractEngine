
#include "ComputeShader.h"
#include "Renderer/Renderer.h"

#if BUILDING_METAL
#include "Platform/Metal/MetalComputeShader.h"
#else
#include "Platform/OpenGL/OpenGLComputeShader.h"
#endif

namespace Graphics {

	Ref<ComputeShader> ComputeShader::Create(const std::filesystem::path& path)
	{
#if BUILDING_METAL
        return CreateRef<MetalComputeShader>(path);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLComputeShader>(path);
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

	Ref<ComputeShader> ComputeShader::Create(const std::string& src)
	{
#if BUILDING_METAL
        return CreateRef<MetalComputeShader>(src, "", false);
#else
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    GRAPHICS_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLComputeShader>(src, "");
		}

		GRAPHICS_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
#endif
	}

#if BUILDING_METAL
    Ref<ComputeShader> ComputeShader::CreateFromMSL(const std::string& MSLSrc, const std::string& functionName)
    {
        return CreateRef<MetalComputeShader>(MSLSrc, functionName, true);
    }
#endif

}
