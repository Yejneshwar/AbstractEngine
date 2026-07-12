#pragma once

#include "Renderer/Texture.h"
#include <Metal/Metal.hpp>

namespace Graphics {

	class MetalTexture2D : public Texture2D
	{
	public:
		MetalTexture2D(uint32_t width, uint32_t height, TextureFormat format, uint32_t mipLevels = 1);
		MetalTexture2D(const std::string& path);
		virtual ~MetalTexture2D();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
        virtual uintptr_t GetRendererID() const override { return (uintptr_t)m_RendererID; }

		virtual const std::string& GetPath() const override { return m_Path; }

		virtual void SetData(void* data, uint32_t size) override;

		virtual void SetMipData(void* data, uint32_t size, uint32_t mip) override;

		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual void Bind(uint32_t slot = 0) const override;

		virtual bool IsLoaded() const override { return m_IsLoaded; }
        
        virtual void Blit(uintptr_t srcTexture) override;

		virtual bool operator==(const Texture& other) const override
		{
			return false;
		}
	private:
        MTL::Texture* m_RendererID;
        MTL::SamplerState* m_SamplerState;
        uint32_t m_Width, m_Height;
        uint32_t m_MipLevels = 1;
        MTL::PixelFormat m_InternalFormat;
        bool m_IsLoaded = false;
        std::string m_Path;
	};

}
