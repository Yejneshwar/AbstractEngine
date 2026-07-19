#pragma once
#ifndef TEXTURE_H
#define TEXTURE_H

#include "GraphicsCore.h"



#include <string>

namespace Graphics {

    enum class TextureFormat
    {
        // Color
        RGBA8,
        RGBA16FLOAT,
        RGBA32FLOAT

    };

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uintptr_t GetRendererID() const = 0;

		virtual const std::string& GetPath() const = 0;

		virtual void SetData(void* data, uint32_t size) = 0;

		// Upload one mip level (for textures created with mipLevels > 1,
		// e.g. the prefiltered environment). `size` is the byte size of the
		// level being uploaded.
		virtual void SetMipData(void* data, uint32_t size, uint32_t mip) = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual bool IsLoaded() const = 0;
        
        virtual void Blit(uintptr_t srcTexture) = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		Texture2D() = delete;
		// mipLevels > 1 allocates a mip chain (uploaded via SetMipData) with a
		// trilinear sampler and horizontal wrap (equirect environment maps
		// need the U seam to filter across).
		static Ref<Texture2D> Create(uint32_t width, uint32_t height, TextureFormat format, uint32_t mipLevels = 1);
		static Ref<Texture2D> Create(const std::string& path);

	protected:
		Texture2D(TextureFormat format) : m_Format(format) {};
		TextureFormat m_Format;
	};

}

#endif
