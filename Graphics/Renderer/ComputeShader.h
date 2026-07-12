#pragma once

#include "GraphicsCore.h"
#include "Shader.h"
#include <filesystem>

namespace Graphics {

	class ComputeShader
	{
	public:
		virtual ~ComputeShader() = default;

		virtual uint32_t GetRendererID() const = 0;

		virtual void Bind() = 0;
        
        virtual void Unbind() = 0;
        
        virtual void BindTexture(uintptr_t texture, int slot) = 0;

        // Bind a texture for sampled reads (GLSL sampler2D / texelFetch)
        // rather than image load/store. On GL this is a texture unit instead
        // of an image unit — required for formats that cannot be bound as
        // images (the depth attachment).
        virtual void BindSampledTexture(uintptr_t texture, int slot) = 0;

        virtual void SetInt(int* ptr, int slot) = 0;

        // Upload a small parameter block to the uniform block bound at
        // `slot` (std140 UBO on GL, setBytes buffer index on Metal).
        virtual void SetData(const void* data, uint32_t size, int slot) = 0;

        virtual void Dispatch(uint32_t width, uint32_t height, uint32_t depth) = 0;

        static Ref<ComputeShader> Create(const std::filesystem::path& path);
        static Ref<ComputeShader> Create(const std::string& src);
#if BUILDING_METAL
        static Ref<ComputeShader> CreateFromMSL(const std::string& MSLSrc, const std::string& functionName);
#endif
	};
}
